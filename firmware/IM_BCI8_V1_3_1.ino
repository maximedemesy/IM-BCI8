/*
  IM-BCI 8 — Firmware V1.3.1
  Arduino-ESP32 / ESP32-S3 + ADS1299
  ------------------------------------------------------------
  Features:
  - ADS1299 8 channels @ 250 SPS
  - 3 sources: Physical / Internal short / Internal test
  - Wi-Fi SoftAP: IM-BCI-8
  - Local web UI: http://192.168.4.1  (mDNS: http://im-bci.local if available)
  - 8 live traces
  - Spectrum + Delta/Theta/Alpha/Beta indicators
  - Electrode labels saved in ESP32 NVS
  - Browser-side CSV recording + experiment markers
  - OTA firmware update from the web UI
  - No third-party Arduino libraries required

  IMPORTANT:
  - Human measurements: battery power ONLY. No USB / bench PSU / grounded scope / generator.
  - Default source at boot = INTERNAL_SHORT so the UI can be validated without electrodes.
  - BIAS defaults OFF. Validate BIAS behavior on the bench before using it on a person.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <SPI.h>
#include <Preferences.h>

// ============================================================
//  USER CONFIG
// ============================================================

#include "secrets.h"

#ifndef IMBCI_AP_PASSWORD
#error "Define IMBCI_AP_PASSWORD in secrets.h"
#endif

#ifndef IMBCI_OTA_PASSWORD
#error "Define IMBCI_OTA_PASSWORD in secrets.h"
#endif

static const char* AP_SSID = "IM-BCI-8";
static const char* AP_PASS = IMBCI_AP_PASSWORD;
static const char* OTA_HOSTNAME = "im-bci-8";
static const char* OTA_USERNAME = "admin";
static const char* OTA_PASSWORD = IMBCI_OTA_PASSWORD;

static constexpr float ADS_VREF_VOLTS = 4.46f; // measured on this prototype

// ESP32-S3 <-> ADS1299 pins validated during bring-up
static constexpr int PIN_START = 8;
static constexpr int PIN_RESET = 9;
static constexpr int PIN_CS    = 10;
static constexpr int PIN_MOSI  = 11;
static constexpr int PIN_SCLK  = 12;
static constexpr int PIN_MISO  = 13;
static constexpr int PIN_DRDY  = 14;

// 1 MHz is deliberately conservative and ample for 8ch @ 250 SPS.
static SPISettings adsSpiSettings(1000000, MSBFIRST, SPI_MODE1);

// ============================================================
//  ADS1299 REGISTERS / COMMANDS
// ============================================================

enum : uint8_t {
  REG_ID         = 0x00,
  REG_CONFIG1    = 0x01,
  REG_CONFIG2    = 0x02,
  REG_CONFIG3    = 0x03,
  REG_LOFF       = 0x04,
  REG_CH1SET     = 0x05,
  REG_CH2SET     = 0x06,
  REG_CH3SET     = 0x07,
  REG_CH4SET     = 0x08,
  REG_CH5SET     = 0x09,
  REG_CH6SET     = 0x0A,
  REG_CH7SET     = 0x0B,
  REG_CH8SET     = 0x0C,
  REG_BIAS_SENSP = 0x0D,
  REG_BIAS_SENSN = 0x0E,
  REG_LOFF_SENSP = 0x0F,
  REG_LOFF_SENSN = 0x10,
  REG_LOFF_FLIP  = 0x11,
  REG_LOFF_STATP = 0x12,
  REG_LOFF_STATN = 0x13,
  REG_GPIO       = 0x14,
  REG_MISC1      = 0x15,
  REG_MISC2      = 0x16,
  REG_CONFIG4    = 0x17
};

enum : uint8_t {
  CMD_WAKEUP = 0x02,
  CMD_STANDBY= 0x04,
  CMD_RESET  = 0x06,
  CMD_START  = 0x08,
  CMD_STOP   = 0x0A,
  CMD_RDATAC = 0x10,
  CMD_SDATAC = 0x11,
  CMD_RDATA  = 0x12
};

// ============================================================
//  RUNTIME STATE
// ============================================================

enum SourceMode : uint8_t {
  SRC_PHYSICAL = 0,
  SRC_SHORT    = 1,
  SRC_TEST     = 2
};

volatile SourceMode sourceMode = SRC_SHORT;
volatile uint8_t userGain = 24;
volatile bool biasEnabled = false;
volatile bool adsReconfiguring = false;

uint8_t adsId = 0x00;

WebServer server(80);
Preferences prefs;
SemaphoreHandle_t adsMutex = nullptr;
TaskHandle_t acquisitionTaskHandle = nullptr;

volatile uint32_t lostDrdyEvents = 0;
volatile uint32_t totalSamples = 0;
volatile bool arduinoOtaInProgress = false;
uint32_t lastOtaProgressPrint = 0;

// ============================================================
//  SAMPLE RING BUFFER
// ============================================================

struct Sample {
  uint32_t seq;
  uint32_t tUs;
  int32_t ch[8];
};

static constexpr uint16_t RING_SIZE = 512;
Sample ringBuffer[RING_SIZE];
volatile uint16_t ringWrite = 0;
volatile uint16_t ringCount = 0;

portMUX_TYPE ringMux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================
//  HELPERS
// ============================================================

static uint8_t gainToCode(uint8_t gain) {
  switch (gain) {
    case 1:  return 0b000;
    case 2:  return 0b001;
    case 4:  return 0b010;
    case 6:  return 0b011;
    case 8:  return 0b100;
    case 12: return 0b101;
    case 24: return 0b110;
    default: return 0b110;
  }
}

static uint8_t activeGain() {
  // Test signal intentionally runs at gain x1 to prevent saturation.
  return (sourceMode == SRC_TEST) ? 1 : userGain;
}

static float currentLsbUv() {
  const float denom = (float)activeGain() * 8388607.0f;
  return (ADS_VREF_VOLTS * 1000000.0f) / denom;
}

static const char* sourceName(SourceMode s) {
  switch (s) {
    case SRC_PHYSICAL: return "physical";
    case SRC_SHORT:    return "short";
    case SRC_TEST:     return "test";
    default:           return "unknown";
  }
}

static void clearRing() {
  portENTER_CRITICAL(&ringMux);
  ringWrite = 0;
  ringCount = 0;
  portEXIT_CRITICAL(&ringMux);
}

// ============================================================
//  ADS1299 LOW-LEVEL
// ============================================================

static void adsSelect() {
  SPI.beginTransaction(adsSpiSettings);
  digitalWrite(PIN_CS, LOW);
}

static void adsDeselect() {
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
}

static void adsCommand(uint8_t cmd) {
  adsSelect();
  SPI.transfer(cmd);
  adsDeselect();
  delayMicroseconds(8);
}

static void adsWriteReg(uint8_t reg, uint8_t value) {
  adsSelect();
  SPI.transfer(0x40 | (reg & 0x1F)); // WREG
  SPI.transfer(0x00);                // one register
  SPI.transfer(value);
  adsDeselect();
  delayMicroseconds(8);
}

static uint8_t adsReadReg(uint8_t reg) {
  adsSelect();
  SPI.transfer(0x20 | (reg & 0x1F)); // RREG
  SPI.transfer(0x00);                // one register
  uint8_t v = SPI.transfer(0x00);
  adsDeselect();
  delayMicroseconds(8);
  return v;
}

static int32_t signExtend24(uint8_t b0, uint8_t b1, uint8_t b2) {
  int32_t v = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
  if (v & 0x00800000) v |= 0xFF000000;
  return v;
}

static bool adsReadFrame(Sample &s) {
  uint8_t data[27];

  adsSelect();
  for (int i = 0; i < 27; ++i) data[i] = SPI.transfer(0x00);
  adsDeselect();

  // bytes 0..2 = status, then 8 x 3-byte signed channels
  for (int ch = 0; ch < 8; ++ch) {
    int i = 3 + ch * 3;
    s.ch[ch] = signExtend24(data[i], data[i + 1], data[i + 2]);
  }
  return true;
}

static void adsHardwareReset() {
  digitalWrite(PIN_START, LOW);
  digitalWrite(PIN_RESET, LOW);
  delayMicroseconds(10);
  digitalWrite(PIN_RESET, HIGH);
  delay(20);
}

static bool adsApplyConfig() {
  if (!adsMutex) return false;

  adsReconfiguring = true;
  xSemaphoreTake(adsMutex, portMAX_DELAY);

  digitalWrite(PIN_START, LOW);
  delay(2);

  adsCommand(CMD_SDATAC);

  // Validated prototype values
  adsWriteReg(REG_CONFIG1, 0x96);  // 250 SPS

  // CONFIG3:
  // 0xE0 = internal reference enabled, BIAS buffer powered down.
  // 0xEC = internal reference enabled + internal BIAS reference + BIAS buffer enabled.
  // Keep BIAS truly OFF unless explicitly requested in physical-input mode.
  const bool useBias = (sourceMode == SRC_PHYSICAL) && biasEnabled;
  adsWriteReg(REG_CONFIG3, useBias ? 0xEC : 0xE0);
  delay(150);                      // reference settling

  // SRB1 is needed for the physical EEG topology only.
  // Keep SRB1 disconnected in INTERNAL_SHORT / INTERNAL_TEST so these
  // diagnostic modes remain fully internal to the ADS1299.
  adsWriteReg(REG_MISC1, (sourceMode == SRC_PHYSICAL) ? 0x20 : 0x00);

  // No lead-off in V1
  adsWriteReg(REG_LOFF_SENSP, 0x00);
  adsWriteReg(REG_LOFF_SENSN, 0x00);
  adsWriteReg(REG_CONFIG4, 0x00);

  uint8_t chset = 0x60; // gain 24, normal input default

  if (sourceMode == SRC_TEST) {
    adsWriteReg(REG_CONFIG2, 0xD0); // internal test signal enabled
    chset = 0x05;                   // gain x1, test mux
  } else {
    adsWriteReg(REG_CONFIG2, 0xC0);
    uint8_t gainCode = gainToCode(userGain);
    chset = (gainCode << 4) | ((sourceMode == SRC_SHORT) ? 0x01 : 0x00);
  }

  for (uint8_t r = REG_CH1SET; r <= REG_CH8SET; ++r) {
    adsWriteReg(r, chset);
  }

  // BIAS V1:
  // Physical-input mode only, positive inputs only, OFF by default.
  // Keep OFF until bench validation with the final electrode system.
  adsWriteReg(REG_BIAS_SENSP, useBias ? 0xFF : 0x00);
  adsWriteReg(REG_BIAS_SENSN, 0x00);

  // Confirm ID while still in SDATAC
  adsId = adsReadReg(REG_ID);

  clearRing();

  adsCommand(CMD_RDATAC);
  delayMicroseconds(10);
  digitalWrite(PIN_START, HIGH);

  xSemaphoreGive(adsMutex);
  adsReconfiguring = false;

  return true;
}

// ============================================================
//  DRDY ISR + ACQUISITION TASK
// ============================================================

void IRAM_ATTR drdyISR() {
  if (adsReconfiguring || acquisitionTaskHandle == nullptr) return;
  BaseType_t highPriorityWoken = pdFALSE;
  vTaskNotifyGiveFromISR(acquisitionTaskHandle, &highPriorityWoken);
  if (highPriorityWoken) portYIELD_FROM_ISR();
}

void acquisitionTask(void *parameter) {
  Sample s{};

  for (;;) {
    // Clear all currently pending DRDY notifications.
    uint32_t pending = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (pending > 1) lostDrdyEvents += (pending - 1);

    if (adsReconfiguring) continue;

    xSemaphoreTake(adsMutex, portMAX_DELAY);

    s.seq = ++totalSamples;
    s.tUs = micros();
    adsReadFrame(s);

    xSemaphoreGive(adsMutex);

    portENTER_CRITICAL(&ringMux);
    ringBuffer[ringWrite] = s;
    ringWrite = (ringWrite + 1) % RING_SIZE;
    if (ringCount < RING_SIZE) ringCount++;
    portEXIT_CRITICAL(&ringMux);
  }
}

// ============================================================
//  ELECTRODE MAP
// ============================================================

static const char* defaultLabels[8] = {
  "Fp1", "Fp2", "C3", "C4", "P7", "P8", "O1", "O2"
};

static String getChannelLabel(int i) {
  char key[5];
  snprintf(key, sizeof(key), "ch%d", i + 1);
  return prefs.getString(key, defaultLabels[i]);
}


// ============================================================
//  ARDUINO OTA
// ============================================================

static void stopAcquisitionForOta() {
  arduinoOtaInProgress = true;
  adsReconfiguring = true;

  // Stop conversions first, then wait until any in-flight SPI read is finished.
  digitalWrite(PIN_START, LOW);
  delay(3);

  if (adsMutex) {
    xSemaphoreTake(adsMutex, portMAX_DELAY);
    xSemaphoreGive(adsMutex);
  }

  Serial.println("[ArduinoOTA] ADS acquisition paused");
}

static void resumeAcquisitionAfterOtaError() {
  // ArduinoOTA normally reboots automatically on success.
  // This path is only for a failed/aborted update so the current firmware stays usable.
  clearRing();
  adsReconfiguring = false;
  arduinoOtaInProgress = false;
  digitalWrite(PIN_START, HIGH);
  Serial.println("[ArduinoOTA] ADS acquisition resumed after OTA error");
}

static bool webOtaAuthorized() {
  return server.authenticate(OTA_USERNAME, OTA_PASSWORD);
}

static void setupArduinoOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setRebootOnSuccess(true);

  ArduinoOTA
    .onStart([]() {
      String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
      Serial.println();
      Serial.println("[ArduinoOTA] ======================================");
      Serial.printf("[ArduinoOTA] Start updating %s\n", type.c_str());
      stopAcquisitionForOta();
    })
    .onEnd([]() {
      Serial.println();
      Serial.println("[ArduinoOTA] Update complete — rebooting");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - lastOtaProgressPrint > 500) {
        unsigned int pct = total ? (unsigned int)((100ULL * progress) / total) : 0;
        Serial.printf("[ArduinoOTA] Progress: %u%%\n", pct);
        lastOtaProgressPrint = millis();
      }
    })
    .onError([](ota_error_t error) {
      Serial.printf("[ArduinoOTA] Error[%u]: ", error);
      if      (error == OTA_AUTH_ERROR)    Serial.println("authentication failed");
      else if (error == OTA_BEGIN_ERROR)   Serial.println("begin failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("connect failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("receive failed");
      else if (error == OTA_END_ERROR)     Serial.println("end failed");
      else                                 Serial.println("unknown");

      resumeAcquisitionAfterOtaError();
    });

  ArduinoOTA.begin();

  // ArduinoOTA owns the mDNS hostname. Add our HTTP service to the same responder.
  MDNS.addService("http", "tcp", 80);

  Serial.printf("[ArduinoOTA] Hostname: %s.local\n", OTA_HOSTNAME);
  Serial.println("[ArduinoOTA] Port: 3232");
  Serial.println("[ArduinoOTA] Ready for Arduino IDE network upload");
}

// ============================================================
//  WEB UI
// ============================================================

static const char INDEX_HTML[] PROGMEM = R"IMBCIHTML(
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>IM-BCI 8 Studio V1.3.1</title>
<style>
:root{
  --bg:#0b0f14;--panel:#121923;--panel2:#0f151e;--line:#263342;
  --text:#edf3f8;--muted:#8da1b5;--accent:#60a5fa;--ok:#5ee6a8;
  --warn:#f8c15c;--danger:#ff6b6b;
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font-family:Inter,system-ui,Arial,sans-serif}
header{height:64px;display:flex;align-items:center;gap:18px;padding:0 22px;border-bottom:1px solid var(--line);background:#0c1118;position:sticky;top:0;z-index:20}
.logo{font-weight:800;font-size:20px;letter-spacing:.5px}
.pill{padding:6px 10px;border:1px solid var(--line);border-radius:999px;color:var(--muted);font-size:12px}
.pill.ok{color:var(--ok)}
.spacer{flex:1}
nav{display:flex;gap:8px;padding:12px 18px;border-bottom:1px solid var(--line);background:#0c1118;position:sticky;top:64px;z-index:19}
nav button{background:transparent;border:1px solid transparent;color:var(--muted);padding:9px 13px;border-radius:9px;cursor:pointer}
nav button.active{background:var(--panel);border-color:var(--line);color:var(--text)}
main{padding:18px;max-width:1500px;margin:auto}
.page{display:none}.page.active{display:block}
.grid{display:grid;grid-template-columns:280px 1fr;gap:16px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:16px;margin-bottom:16px}
.card h2,.card h3{margin:0 0 12px}
label{display:block;color:var(--muted);font-size:12px;margin-bottom:5px}
select,input,button{font:inherit}
select,input[type=text],input[type=number],input[type=file]{width:100%;background:var(--panel2);color:var(--text);border:1px solid var(--line);padding:9px;border-radius:8px}
.btn{border:1px solid var(--line);background:#192433;color:var(--text);padding:9px 12px;border-radius:8px;cursor:pointer}
.btn:hover{filter:brightness(1.12)}
.btn.primary{background:#173d67;border-color:#2a69a2}
.btn.warn{background:#4d3815;border-color:#826022}
.btn.danger{background:#552020;border-color:#873232}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.stack>*{margin-bottom:10px}
.small{font-size:12px;color:var(--muted)}
.kpi{display:grid;grid-template-columns:repeat(4,1fr);gap:10px}
.kpi div{background:var(--panel2);border:1px solid var(--line);padding:12px;border-radius:10px}
.kpi b{display:block;font-size:18px;margin-top:4px}
canvas{width:100%;background:#090d12;border:1px solid var(--line);border-radius:10px}
#liveCanvas{height:900px}
#specCanvas{height:340px}
.bandgrid{display:grid;grid-template-columns:repeat(4,1fr);gap:10px}
.band{background:var(--panel2);border:1px solid var(--line);border-radius:10px;padding:12px}
.band .bar{height:7px;background:#263342;border-radius:6px;margin-top:8px;overflow:hidden}
.band .bar span{display:block;height:100%;background:var(--accent);width:0%}
.electrodes{display:grid;grid-template-columns:repeat(4,1fr);gap:10px}
.markerlog{height:220px;overflow:auto;background:#090d12;border:1px solid var(--line);padding:10px;border-radius:8px;font-family:ui-monospace,monospace;font-size:12px}
.notice{border-left:3px solid var(--warn);padding:10px 12px;background:#2a2215;color:#f7deb0;border-radius:6px}
.good{border-left-color:var(--ok);background:#13261f;color:#bef7dc}
.dangerbox{border-left:3px solid var(--danger);padding:10px 12px;background:#2b1717;color:#ffd0d0;border-radius:6px}
.statusdot{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--ok);margin-right:6px}
.headset-figure{display:grid;grid-template-columns:1fr 1fr;gap:16px}
.headset-box{background:var(--panel2);border:1px solid var(--line);border-radius:12px;padding:12px}
.headset-box h4{margin:0 0 10px}
.svgwrap{width:100%;overflow:auto}
.svgnote{font-size:12px;color:var(--muted);margin-top:8px}
.node-label{font-size:12px;font-weight:700;fill:#edf3f8;text-anchor:middle}
.node-label-small{font-size:11px;font-weight:600;fill:#b9c9d8;text-anchor:middle}
.channel-dot{stroke:#0b0f14;stroke-width:2}
.ref-dot{fill:#f8c15c;stroke:#0b0f14;stroke-width:2}
.bias-dot{fill:#ff7b7b;stroke:#0b0f14;stroke-width:2}
.mapcaption{font-size:12px;fill:#93a6b8;text-anchor:middle}
.live-help{font-size:12px;color:var(--muted);line-height:1.45}
.live-help b{color:var(--text)}
@media(max-width:900px){.grid{grid-template-columns:1fr}.kpi,.bandgrid,.electrodes{grid-template-columns:repeat(2,1fr)}header{gap:8px}.pill.hide-sm{display:none}.headset-figure{grid-template-columns:1fr}}
</style>
</head>
<body>
<header>
  <div class="logo">IM-BCI 8 Studio <span class="small">V1.3.1</span></div>
  <div class="pill ok" id="adsPill"><span class="statusdot"></span>ADS --</div>
  <div class="pill">250 SPS</div>
  <div class="pill hide-sm" id="sourcePill">source --</div>
  <div class="pill hide-sm" id="lossPill">lost 0</div>
  <div class="spacer"></div>
  <div class="pill" id="wifiPill">192.168.4.1</div>
</header>

<nav>
  <button data-page="live" class="active">LIVE</button>
  <button data-page="spectrum">SPECTRUM</button>
  <button data-page="headset">HEADSET</button>
  <button data-page="experiments">EXPERIMENTS</button>
  <button data-page="system">SYSTEM</button>
</nav>

<main>
<section id="page-live" class="page active">
  <div class="grid">
    <div>
      <div class="card stack">
        <h3>Acquisition</h3>
        <div>
          <label>Source ADS1299</label>
          <select id="sourceSel">
            <option value="short">Internal short</option>
            <option value="test">Internal test signal</option>
            <option value="physical">Physical inputs</option>
          </select>
        </div>
        <div>
          <label>Gain (Physical / Short)</label>
          <select id="gainSel">
            <option>1</option><option>2</option><option>4</option><option>6</option>
            <option>8</option><option>12</option><option selected>24</option>
          </select>
        </div>
        <div class="row">
          <input type="checkbox" id="biasToggle" style="width:auto">
          <label for="biasToggle" style="margin:0">BIAS (expérimental)</label>
        </div>
        <button class="btn primary" id="applyCfg">Appliquer</button>
        <div class="notice">Pour le développement sans électrodes, commence par <b>Internal short</b>, puis <b>Internal test</b>. Les entrées physiques flottantes capteront surtout le bruit ambiant.</div>
      </div>

      <div class="card stack">
        <h3>Affichage</h3>
        <div class="row"><input type="checkbox" id="dcToggle" checked style="width:auto"><label for="dcToggle" style="margin:0">Retirer la moyenne affichée</label></div>
        <div class="row"><input type="checkbox" id="notchToggle" checked style="width:auto"><label for="notchToggle" style="margin:0">Notch 50 Hz</label></div>
        <div>
          <label>Mode d’échelle</label>
          <select id="scaleMode">
            <option value="common" selected>Échelle commune — comparaison fiable</option>
            <option value="individual">Auto individuel — diagnostic canal</option>
          </select>
        </div>
        <div>
          <label>Échelle commune ± µV</label>
          <select id="scaleSel">
            <option value="5">5</option>
            <option value="20">20</option>
            <option value="100">100</option>
            <option value="500">500</option>
            <option value="2000" selected>2000</option>
            <option value="10000">10000</option>
          </select>
        </div>
        <button class="btn" id="autoScale">Auto-ajuster</button>
        <div class="live-help">
          <b>Échelle commune</b> : tous les canaux utilisent la même amplitude, idéal pour comparer les voies.<br>
          <b>Auto individuel</b> : chaque canal reçoit sa propre échelle pour repérer rapidement un mauvais contact.
        </div>
      </div>

      <div class="card">
        <h3>Session</h3>
        <div class="row">
          <button class="btn primary" id="recordBtn">● Enregistrer</button>
          <button class="btn" id="exportBtn" disabled>Exporter CSV</button>
        </div>
        <div class="small" id="recordInfo">Aucun enregistrement.</div>
      </div>
    </div>

    <div>
      <div class="card">
        <div class="row" style="justify-content:space-between;margin-bottom:10px">
          <div><b>EEG temps réel — 8 voies séparées</b> <span class="small">• fenêtre 5 s</span></div>
          <div class="small" id="liveStats">--</div>
        </div>
        <canvas id="liveCanvas" width="1200" height="900"></canvas>
      </div>
    </div>
  </div>
</section>

<section id="page-spectrum" class="page">
  <div class="card">
    <div class="row">
      <h2 style="margin-right:auto">Spectre / bandes EEG</h2>
      <label style="margin:0">Canal</label>
      <select id="specChannel" style="width:150px"></select>
    </div>
    <canvas id="specCanvas" width="1200" height="340"></canvas>
  </div>
  <div class="bandgrid">
    <div class="band">Delta 0.5–4 Hz <b id="bandDelta">--</b><div class="bar"><span id="barDelta"></span></div></div>
    <div class="band">Theta 4–8 Hz <b id="bandTheta">--</b><div class="bar"><span id="barTheta"></span></div></div>
    <div class="band">Alpha 8–12 Hz <b id="bandAlpha">--</b><div class="bar"><span id="barAlpha"></span></div></div>
    <div class="band">Beta 13–30 Hz <b id="bandBeta">--</b><div class="bar"><span id="barBeta"></span></div></div>
  </div>
  <div class="card">
    <div class="notice good">Ces indicateurs sont destinés à l’expérimentation. Le premier test recommandé est une comparaison O1/O2 yeux ouverts / yeux fermés et non une interprétation clinique.</div>
  </div>
</section>

<section id="page-headset" class="page">
  <div class="card">
    <h2>Placement des électrodes</h2>
    <p class="small">Les labels sont sauvegardés dans la mémoire NVS de l’ESP32. Ils servent aux courbes, au spectre et aux exports CSV.</p>
    <div class="electrodes" id="electrodeGrid"></div>
    <div class="row" style="margin-top:14px">
      <button class="btn primary" id="saveMap">Sauvegarder sur IM-BCI 8</button>
      <button class="btn" id="defaultMap">Placement initial</button>
    </div>
  </div>

  <div class="card">
    <h3>Schémas de placement</h3>
    <div class="headset-figure">
      <div class="headset-box">
        <h4>Montage IM-BCI 8 proposé</h4>
        <div class="svgwrap">
          <svg viewBox="0 0 360 430" width="100%" role="img" aria-label="Montage IM-BCI 8">
            <defs>
              <linearGradient id="headgrad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stop-color="#172331"/>
                <stop offset="100%" stop-color="#0e151d"/>
              </linearGradient>
            </defs>
            <ellipse cx="180" cy="205" rx="120" ry="155" fill="url(#headgrad)" stroke="#314455" stroke-width="2"/>
            <ellipse cx="50" cy="205" rx="18" ry="42" fill="none" stroke="#314455" stroke-width="2"/>
            <ellipse cx="310" cy="205" rx="18" ry="42" fill="none" stroke="#314455" stroke-width="2"/>
            <path d="M180 70 L170 50 L190 50 Z" fill="#314455"/>
            <line x1="180" y1="48" x2="180" y2="28" stroke="#506272" stroke-width="2"/>
            <text x="180" y="20" class="mapcaption">Avant / nez</text>

            <circle cx="130" cy="110" r="11" fill="#60a5fa" class="channel-dot"/>
            <circle cx="230" cy="110" r="11" fill="#5ee6a8" class="channel-dot"/>
            <circle cx="125" cy="195" r="11" fill="#f8c15c" class="channel-dot"/>
            <circle cx="235" cy="195" r="11" fill="#d78cff" class="channel-dot"/>
            <circle cx="95" cy="270" r="11" fill="#ff7b7b" class="channel-dot"/>
            <circle cx="265" cy="270" r="11" fill="#67d7e8" class="channel-dot"/>
            <circle cx="140" cy="330" r="11" fill="#f59fe7" class="channel-dot"/>
            <circle cx="220" cy="330" r="11" fill="#b5e06c" class="channel-dot"/>

            <text id="svgLbl1" x="130" y="95" class="node-label">Fp1</text>
            <text id="svgLbl2" x="230" y="95" class="node-label">Fp2</text>
            <text id="svgLbl3" x="125" y="180" class="node-label">C3</text>
            <text id="svgLbl4" x="235" y="180" class="node-label">C4</text>
            <text id="svgLbl5" x="95"  y="255" class="node-label">P7</text>
            <text id="svgLbl6" x="265" y="255" class="node-label">P8</text>
            <text id="svgLbl7" x="140" y="315" class="node-label">O1</text>
            <text id="svgLbl8" x="220" y="315" class="node-label">O2</text>

            <circle cx="46" cy="205" r="10" class="ref-dot"/>
            <circle cx="314" cy="205" r="10" class="bias-dot"/>
            <text x="46" y="185" class="node-label-small">REF</text>
            <text x="314" y="185" class="node-label-small">BIAS</text>
            <text x="46" y="224" class="node-label-small">A1</text>
            <text x="314" y="224" class="node-label-small">A2</text>
          </svg>
        </div>
        <div class="svgnote">Vue de dessus simplifiée. Les labels des 8 canaux sont mis à jour automatiquement selon le mapping CH1→CH8 défini ci-dessus.</div>
      </div>

      <div class="headset-box">
        <h4>Repère 10–20 simplifié</h4>
        <div class="svgwrap">
          <svg viewBox="0 0 360 430" width="100%" role="img" aria-label="Repère 10-20 simplifié">
            <ellipse cx="180" cy="205" rx="120" ry="155" fill="none" stroke="#314455" stroke-width="2"/>
            <ellipse cx="50" cy="205" rx="18" ry="42" fill="none" stroke="#314455" stroke-width="2"/>
            <ellipse cx="310" cy="205" rx="18" ry="42" fill="none" stroke="#314455" stroke-width="2"/>
            <path d="M180 70 L170 50 L190 50 Z" fill="#314455"/>
            <line x1="180" y1="60" x2="180" y2="350" stroke="#263342" stroke-width="1.5" stroke-dasharray="5 5"/>
            <path d="M75 120 Q180 95 285 120" fill="none" stroke="#263342" stroke-width="1.5" stroke-dasharray="5 5"/>
            <path d="M70 205 Q180 180 290 205" fill="none" stroke="#263342" stroke-width="1.5" stroke-dasharray="5 5"/>
            <path d="M85 290 Q180 270 275 290" fill="none" stroke="#263342" stroke-width="1.5" stroke-dasharray="5 5"/>

            <g fill="#b9c9d8" font-size="12" font-weight="700" text-anchor="middle">
              <text x="125" y="92">Fp1</text><text x="180" y="82">Fpz</text><text x="235" y="92">Fp2</text>
              <text x="92"  y="132">F7</text><text x="135" y="132">F3</text><text x="180" y="126">Fz</text><text x="225" y="132">F4</text><text x="268" y="132">F8</text>
              <text x="75"  y="205">T7</text><text x="125" y="205">C3</text><text x="180" y="198">Cz</text><text x="235" y="205">C4</text><text x="285" y="205">T8</text>
              <text x="92"  y="278">P7</text><text x="135" y="278">P3</text><text x="180" y="270">Pz</text><text x="225" y="278">P4</text><text x="268" y="278">P8</text>
              <text x="135" y="338">O1</text><text x="180" y="348">Oz</text><text x="225" y="338">O2</text>
              <text x="46" y="205">A1</text><text x="314" y="205">A2</text>
            </g>

            <g fill="#506272">
              <circle cx="125" cy="96" r="3"/><circle cx="180" cy="86" r="3"/><circle cx="235" cy="96" r="3"/>
              <circle cx="92" cy="136" r="3"/><circle cx="135" cy="136" r="3"/><circle cx="180" cy="130" r="3"/><circle cx="225" cy="136" r="3"/><circle cx="268" cy="136" r="3"/>
              <circle cx="75" cy="209" r="3"/><circle cx="125" cy="209" r="3"/><circle cx="180" cy="202" r="3"/><circle cx="235" cy="209" r="3"/><circle cx="285" cy="209" r="3"/>
              <circle cx="92" cy="282" r="3"/><circle cx="135" cy="282" r="3"/><circle cx="180" cy="274" r="3"/><circle cx="225" cy="282" r="3"/><circle cx="268" cy="282" r="3"/>
              <circle cx="135" cy="342" r="3"/><circle cx="180" cy="352" r="3"/><circle cx="225" cy="342" r="3"/>
              <circle cx="46" cy="209" r="3"/><circle cx="314" cy="209" r="3"/>
            </g>
          </svg>
        </div>
        <div class="svgnote">Repère indicatif des positions EEG classiques les plus utiles pour un montage 8 canaux : frontales, centrales, pariétales, occipitales et références oreilles A1/A2.</div>
      </div>
    </div>
  </div>

  <div class="card">
    <h3>Placement V1 proposé</h3>
    <div class="small">CH1 Fp1 • CH2 Fp2 • CH3 C3 • CH4 C4 • CH5 P7 • CH6 P8 • CH7 O1 • CH8 O2 • REF et BIAS aux oreilles.</div>
  </div>
</section>

<section id="page-experiments" class="page">
  <div class="grid">
    <div>
      <div class="card">
        <h3>Marqueurs</h3>
        <div class="row">
          <button class="btn" data-marker="EYES_OPEN">Eyes open</button>
          <button class="btn" data-marker="EYES_CLOSED">Eyes closed</button>
          <button class="btn" data-marker="BLINK">Blink</button>
          <button class="btn" data-marker="STIMULUS">Stimulus</button>
        </div>
        <div style="margin-top:10px">
          <label>Marqueur personnalisé</label>
          <div class="row"><input id="customMarker" type="text" placeholder="ex. LEFT_HAND"><button class="btn" id="addCustomMarker">Ajouter</button></div>
        </div>
      </div>
      <div class="card">
        <h3>BCI Lab — V1</h3>
        <div class="small">Cette V1 n’actionne pas encore le clavier global de Windows. Elle fournit surtout le dataset CSV + marqueurs nécessaire pour entraîner et valider des détecteurs/classifieurs sur PC.</div>
        <div class="notice" style="margin-top:10px">Étape suivante : bridge Python → BrainFlow/MNE → classifier → clavier / souris / joystick virtuel / OSC.</div>
      </div>
    </div>
    <div class="card">
      <h3>Journal de session</h3>
      <div class="markerlog" id="markerLog"></div>
    </div>
  </div>
</section>

<section id="page-system" class="page">
  <div class="kpi">
    <div><span class="small">ADS1299</span><b id="kpiAds">--</b></div>
    <div><span class="small">Source</span><b id="kpiSource">--</b></div>
    <div><span class="small">Échantillons</span><b id="kpiSamples">--</b></div>
    <div><span class="small">Heap ESP32</span><b id="kpiHeap">--</b></div>
  </div>

  <div class="card">
    <h2>OTA / mises à jour</h2>
    <div class="notice good">
      <b>Développement recommandé :</b> connecte le PC au Wi-Fi IM-BCI-8, sélectionne le port réseau
      <b>im-bci-8</b> dans Arduino IDE puis clique simplement sur Upload. Arduino IDE compile et transmet
      automatiquement le firmware par ArduinoOTA.
    </div>
    <p class="small">OTA web de secours : Sketch → Export Compiled Binary, puis sélectionner le .bin ci-dessous.</p>
    <label>Mot de passe OTA</label>
    <input type="password" id="otaPassword" autocomplete="current-password" placeholder="Mot de passe OTA">
    <input type="file" id="otaFile" accept=".bin,application/octet-stream" style="margin-top:10px">
    <button class="btn warn" style="margin-top:10px" id="otaBtn">Mettre à jour le firmware</button>
    <div class="small" id="otaInfo"></div>
  </div>

  <div class="card">
    <h3>Sécurité</h3>
    <div class="dangerbox"><b>Mesure sur une personne = batterie uniquement.</b><br>Aucun USB, alimentation de laboratoire, oscilloscope ou générateur relié pendant le port des électrodes.</div>
  </div>
</section>
</main>

<script>
const FS=250, MAX_POINTS=1250;
let labels=["Fp1","Fp2","C3","C4","P7","P8","O1","O2"];
let raw=Array.from({length:8},()=>[]);
let lastSeq=0, lsbUv=0.022153, polling=false;
let recording=false, recorded=[], pendingMarker="", sessionStart=0;
let markerEvents=[];
const LIVE_SCALE_OPTIONS=[5,20,100,500,2000,10000];
let individualScales=Array(8).fill(2000);

const $=id=>document.getElementById(id);

document.querySelectorAll("nav button").forEach(b=>{
  b.onclick=()=>{
    document.querySelectorAll("nav button").forEach(x=>x.classList.remove("active"));
    document.querySelectorAll(".page").forEach(x=>x.classList.remove("active"));
    b.classList.add("active"); $("page-"+b.dataset.page).classList.add("active");
  };
});

function fmtHex(v){return "0x"+Number(v).toString(16).toUpperCase().padStart(2,"0")}

async function status(){
  try{
    const r=await fetch("/api/status"); const s=await r.json();
    $("adsPill").innerHTML='<span class="statusdot"></span>ADS '+fmtHex(s.adsId);
    $("sourcePill").textContent="source "+s.source;
    $("lossPill").textContent="lost "+s.lost;
    $("wifiPill").textContent=s.ip;
    $("kpiAds").textContent=fmtHex(s.adsId);
    $("kpiSource").textContent=s.source+" ×"+s.gain;
    $("kpiSamples").textContent=s.samples;
    $("kpiHeap").textContent=Math.round(s.heap/1024)+" kB";
    // Do not overwrite local form selections on every status refresh.
    // The user may be changing source/gain/BIAS before pressing "Appliquer".
    lsbUv=s.lsbUv;
  }catch(e){}
}

async function pollData(){
  if(polling) return;
  polling=true;
  try{
    const r=await fetch(`/api/data?after=${lastSeq}&max=32`,{cache:"no-store"});
    const d=await r.json();
    lsbUv=d.lsbUv;
    for(const smp of d.samples){
      lastSeq=Math.max(lastSeq,smp[0]);
      const t=smp[1];
      for(let c=0;c<8;c++){
        raw[c].push(smp[c+2]*lsbUv);
        if(raw[c].length>MAX_POINTS) raw[c].splice(0,raw[c].length-MAX_POINTS);
      }
      if(recording){
        let row=[smp[0],t];
        for(let c=0;c<8;c++) row.push(smp[c+2]*lsbUv);
        row.push(pendingMarker);
        pendingMarker="";
        recorded.push(row);
      }
    }
    $("liveStats").textContent=`seq ${lastSeq} • ${d.samples.length} new • ${lsbUv.toFixed(6)} µV/count`;
  }catch(e){}
  polling=false;
}
setInterval(pollData,80);
setInterval(status,1000);

function notch50(arr){
  if(!$("notchToggle").checked || arr.length<3) return arr.slice();
  const f0=50,Q=30,w0=2*Math.PI*f0/FS;
  const alpha=Math.sin(w0)/(2*Q), a0=1+alpha;
  const b0=1/a0,b1=-2*Math.cos(w0)/a0,b2=1/a0;
  const a1=-2*Math.cos(w0)/a0,a2=(1-alpha)/a0;
  let y=new Array(arr.length),x1=0,x2=0,y1=0,y2=0;
  for(let i=0;i<arr.length;i++){
    const x=arr[i];
    const o=b0*x+b1*x1+b2*x2-a1*y1-a2*y2;
    y[i]=o;x2=x1;x1=x;y2=y1;y1=o;
  }
  return y;
}

function processed(c){
  let a=notch50(raw[c]);
  if($("dcToggle").checked && a.length){
    const m=a.reduce((x,y)=>x+y,0)/a.length;
    a=a.map(v=>v-m);
  }
  return a;
}

function niceScaleFor(values){
  if(!values || !values.length) return 5;
  const abs=values.map(v=>Math.abs(v)).sort((a,b)=>a-b);
  // 98e percentile: évite qu'un seul artefact énorme rende la trace illisible.
  const idx=Math.min(abs.length-1,Math.floor(abs.length*0.98));
  const target=Math.max(1,abs[idx]*1.20);
  return LIVE_SCALE_OPTIONS.find(x=>x>=target) || 10000;
}

function channelMetrics(a){
  if(!a.length) return {rms:0,pp:0,last:0};
  const n=Math.min(FS,a.length); // métriques sur la dernière seconde
  const b=a.slice(a.length-n);
  let ss=0,mn=Infinity,mx=-Infinity;
  for(const v of b){
    ss+=v*v;
    if(v<mn)mn=v;
    if(v>mx)mx=v;
  }
  return {
    rms:Math.sqrt(ss/b.length),
    pp:mx-mn,
    last:b[b.length-1]
  };
}

function fmtUv(v){
  const av=Math.abs(v);
  if(av>=1000) return v.toFixed(0);
  if(av>=100) return v.toFixed(1);
  if(av>=10) return v.toFixed(2);
  return v.toFixed(3);
}

function drawLive(){
  const cv=$("liveCanvas"), ctx=cv.getContext("2d");
  const W=cv.width,H=cv.height;
  const left=122,right=18,top=12,bottom=36,gap=7;
  const plotX=left, plotW=W-left-right;
  const available=H-top-bottom-gap*7;
  const bandH=available/8;
  const commonScale=Number($("scaleSel").value);
  const individual=$("scaleMode").value==="individual";
  const palette=["#60a5fa","#5ee6a8","#f8c15c","#d78cff","#ff7b7b","#67d7e8","#f59fe7","#b5e06c"];

  ctx.fillStyle="#090d12";
  ctx.fillRect(0,0,W,H);
  ctx.font="12px system-ui";

  for(let c=0;c<8;c++){
    const a=processed(c);
    const scale=individual ? individualScales[c] : commonScale;
    const y0=top+c*(bandH+gap);
    const y1=y0+bandH;
    const mid=(y0+y1)/2;
    const innerTop=y0+8, innerBottom=y1-8;
    const half=(innerBottom-innerTop)/2;

    // Fond propre par canal
    ctx.fillStyle=(c%2===0)?"#0d131b":"#0b1118";
    ctx.fillRect(0,y0,W,bandH);

    // Séparateur et zéro
    ctx.strokeStyle="#233140";
    ctx.lineWidth=1;
    ctx.beginPath();ctx.moveTo(0,y1);ctx.lineTo(W,y1);ctx.stroke();

    ctx.strokeStyle="#34485b";
    ctx.setLineDash([5,5]);
    ctx.beginPath();ctx.moveTo(plotX,mid);ctx.lineTo(W-right,mid);ctx.stroke();
    ctx.setLineDash([]);

    // Grille temporelle commune : -5 ... 0 s
    ctx.strokeStyle="#17222e";
    for(let k=0;k<=5;k++){
      const x=plotX+(k/5)*plotW;
      ctx.beginPath();ctx.moveTo(x,y0);ctx.lineTo(x,y1);ctx.stroke();
    }

    // Identité canal
    ctx.fillStyle=palette[c];
    ctx.font="bold 14px system-ui";
    ctx.fillText(`CH${c+1}`,8,y0+22);

    ctx.fillStyle="#edf3f8";
    ctx.font="bold 13px system-ui";
    ctx.fillText(labels[c]||"",47,y0+22);

    // Echelle locale visible
    ctx.fillStyle="#93a6b8";
    ctx.font="11px ui-monospace,monospace";
    ctx.textAlign="right";
    ctx.fillText(`+${scale}`,plotX-8,innerTop+4);
    ctx.fillText("0 µV",plotX-8,mid+4);
    ctx.fillText(`-${scale}`,plotX-8,innerBottom+3);
    ctx.textAlign="left";

    // Métriques de la dernière seconde
    const m=channelMetrics(a);
    ctx.fillStyle="#93a6b8";
    ctx.font="11px ui-monospace,monospace";
    const metric=`RMS ${fmtUv(m.rms)} µV   P-P ${fmtUv(m.pp)} µV`;
    const tw=ctx.measureText(metric).width;
    ctx.fillText(metric,W-right-tw,y0+16);

    if(a.length>=2){
      // Détection saturation visuelle
      let clipped=false;

      // La trace est CLIPPÉE à son propre panneau : elle ne peut plus croiser les autres.
      ctx.save();
      ctx.beginPath();
      ctx.rect(plotX,y0,plotW,bandH);
      ctx.clip();

      ctx.strokeStyle=palette[c];
      ctx.lineWidth=1.35;
      ctx.beginPath();
      for(let i=0;i<a.length;i++){
        const x=plotX+(i/(MAX_POINTS-1))*plotW;
        const norm=a[i]/scale;
        if(Math.abs(norm)>1) clipped=true;
        const y=mid-norm*half;
        if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
      }
      ctx.stroke();
      ctx.restore();

      if(clipped){
        ctx.fillStyle="#ff7b7b";
        ctx.font="bold 11px system-ui";
        ctx.fillText("HORS ÉCHELLE",plotX+8,y0+16);
      }
    }
  }

  // Axe temporel unique
  ctx.fillStyle="#93a6b8";
  ctx.font="11px system-ui";
  ctx.textAlign="center";
  for(let k=0;k<=5;k++){
    const x=plotX+(k/5)*plotW;
    const sec=-5+k;
    ctx.fillText(sec===0?"0 s":`${sec} s`,x,H-14);
  }
  ctx.textAlign="left";

  requestAnimationFrame(drawLive);
}
requestAnimationFrame(drawLive);

function refreshIndividualScales(){
  for(let c=0;c<8;c++){
    individualScales[c]=niceScaleFor(processed(c));
  }
}

$("autoScale").onclick=()=>{
  if($("scaleMode").value==="individual"){
    refreshIndividualScales();
  }else{
    let all=[];
    for(let c=0;c<8;c++) all=all.concat(processed(c));
    $("scaleSel").value=String(niceScaleFor(all));
  }
};

$("scaleMode").onchange=()=>{
  const individual=$("scaleMode").value==="individual";
  $("scaleSel").disabled=individual;
  $("autoScale").textContent=individual?"Auto-ajuster les 8 voies":"Auto-ajuster l’échelle commune";
  if(individual) refreshIndividualScales();
};

$("applyCfg").onclick=async()=>{
  const body=new URLSearchParams({
    source:$("sourceSel").value,
    gain:$("gainSel").value,
    bias:$("biasToggle").checked?"1":"0"
  });
  await fetch("/api/config",{method:"POST",body});
  raw.forEach(a=>a.length=0); lastSeq=0;
  await status();
};

function buildElectrodeGrid(){
  const g=$("electrodeGrid");g.innerHTML="";
  for(let i=0;i<8;i++){
    const d=document.createElement("div");
    d.innerHTML=`<label>CH${i+1}</label><input type="text" id="map${i}" value="${labels[i]}">`;
    g.appendChild(d);
  }
}

async function loadMap(){
  try{
    const r=await fetch("/api/map");const j=await r.json();labels=j.labels;
  }catch(e){}
  buildElectrodeGrid();refreshSpecChannels();updateHeadsetSvg();
}

$("saveMap").onclick=async()=>{
  const p=new URLSearchParams();
  for(let i=0;i<8;i++){labels[i]=$("map"+i).value.trim()||("CH"+(i+1));p.set("ch"+(i+1),labels[i]);}
  await fetch("/api/map",{method:"POST",body:p});refreshSpecChannels();updateHeadsetSvg();
};

$("defaultMap").onclick=()=>{
  labels=["Fp1","Fp2","C3","C4","P7","P8","O1","O2"];buildElectrodeGrid();refreshSpecChannels();updateHeadsetSvg();
};

function updateHeadsetSvg(){
  for(let i=0;i<8;i++){
    const el=$("svgLbl"+(i+1));
    if(el) el.textContent = labels[i];
  }
}

function refreshSpecChannels(){
  const s=$("specChannel"), old=s.value;s.innerHTML="";
  for(let i=0;i<8;i++){let o=document.createElement("option");o.value=i;o.textContent=`CH${i+1} ${labels[i]}`;s.appendChild(o);}
  if(old!=="")s.value=old;
}

function dftSpectrum(arr){
  const N=256;
  if(arr.length<N)return null;
  const x=arr.slice(-N);
  const m=x.reduce((a,b)=>a+b,0)/N;
  for(let i=0;i<N;i++) x[i]=(x[i]-m)*(0.5-0.5*Math.cos(2*Math.PI*i/(N-1)));
  const out=[];
  for(let k=1;k<=Math.floor(N/2);k++){
    let re=0,im=0;
    for(let n=0;n<N;n++){
      const a=2*Math.PI*k*n/N;re+=x[n]*Math.cos(a);im-=x[n]*Math.sin(a);
    }
    out.push({f:k*FS/N,p:(re*re+im*im)/(N*N)});
  }
  return out;
}

function drawSpectrum(){
  const c=Number($("specChannel").value||0);
  const sp=dftSpectrum(processed(c));
  const cv=$("specCanvas"),ctx=cv.getContext("2d"),W=cv.width,H=cv.height;
  ctx.fillStyle="#090d12";ctx.fillRect(0,0,W,H);
  if(!sp){ctx.fillStyle="#8da1b5";ctx.fillText("Collecte de données…",20,30);return;}
  const view=sp.filter(x=>x.f<=60);
  const db=view.map(x=>10*Math.log10(x.p+1e-12));
  let max=Math.max(...db),min=max-70;
  ctx.strokeStyle="#263342";ctx.fillStyle="#8da1b5";ctx.font="12px system-ui";
  [0,10,20,30,40,50,60].forEach(f=>{const x=f/60*W;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,H);ctx.stroke();ctx.fillText(f+" Hz",x+3,H-8)});
  ctx.strokeStyle="#60a5fa";ctx.lineWidth=1.5;ctx.beginPath();
  view.forEach((v,i)=>{const x=v.f/60*W,y=H-20-(db[i]-min)/(max-min)*(H-35);if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y)});ctx.stroke();

  const bands={Delta:[.5,4],Theta:[4,8],Alpha:[8,12],Beta:[13,30]};
  let vals={},sum=0;
  for(const [name,[lo,hi]] of Object.entries(bands)){
    let p=view.filter(x=>x.f>=lo&&x.f<hi).reduce((a,b)=>a+b.p,0); vals[name]=p;sum+=p;
  }
  for(const n of Object.keys(vals)){
    const pct=sum?100*vals[n]/sum:0;
    $("band"+n).textContent=pct.toFixed(1)+" %";$("bar"+n).style.width=pct+"%";
  }
}
setInterval(drawSpectrum,500);

$("recordBtn").onclick=()=>{
  recording=!recording;
  if(recording){
    recorded=[];markerEvents=[];pendingMarker="";sessionStart=performance.now();
    $("recordBtn").textContent="■ Arrêter";$("recordBtn").classList.add("danger");$("exportBtn").disabled=true;
    logMarker("RECORD_START",false);
  }else{
    logMarker("RECORD_STOP",false);
    $("recordBtn").textContent="● Enregistrer";$("recordBtn").classList.remove("danger");$("exportBtn").disabled=recorded.length===0;
  }
};

function logMarker(name,attach=true){
  const ms=recording?(performance.now()-sessionStart):0;
  markerEvents.push({ms,name});
  if(attach)pendingMarker=name;
  const div=document.createElement("div");div.textContent=`${(ms/1000).toFixed(3)} s  ${name}`;
  $("markerLog").appendChild(div);$("markerLog").scrollTop=$("markerLog").scrollHeight;
}

document.querySelectorAll("[data-marker]").forEach(b=>b.onclick=()=>logMarker(b.dataset.marker));
$("addCustomMarker").onclick=()=>{const v=$("customMarker").value.trim();if(v){logMarker(v);$("customMarker").value=""}};

$("exportBtn").onclick=()=>{
  if(!recorded.length)return;
  let csv="seq,esp_us,"+labels.map((x,i)=>`CH${i+1}_${x}_uV`).join(",")+",marker\n";
  for(const r of recorded){
    csv+=r.slice(0,-1).map(v=>typeof v==="number"?v.toFixed?.(6)??v:v).join(",")+',"'+String(r[r.length-1]).replaceAll('"','""')+'"\n';
  }
  const blob=new Blob([csv],{type:"text/csv"}),url=URL.createObjectURL(blob),a=document.createElement("a");
  a.href=url;a.download=`IM-BCI8_${new Date().toISOString().replaceAll(":","-")}.csv`;a.click();URL.revokeObjectURL(url);
  $("recordInfo").textContent=`${recorded.length} échantillons exportés.`;
};

$("otaBtn").onclick=async()=>{
  const f=$("otaFile").files[0];
  const pw=$("otaPassword").value;
  if(!f){$("otaInfo").textContent="Sélectionne d'abord un .bin";return;}
  if(!pw){$("otaInfo").textContent="Saisis le mot de passe OTA";return;}
  const fd=new FormData();fd.append("firmware",f);
  $("otaInfo").textContent="Upload en cours…";
  try{
    const auth="Basic "+btoa("admin:"+pw);
    const r=await fetch("/update",{method:"POST",body:fd,headers:{Authorization:auth}});
    if(r.status===401){$("otaInfo").textContent="Mot de passe OTA incorrect";return;}
    $("otaInfo").textContent=await r.text();
  }catch(e){$("otaInfo").textContent="Connexion interrompue : le redémarrage peut être normal après OTA."}
};

async function syncControlsFromDevice(){
  try{
    const r=await fetch("/api/status"); const s=await r.json();
    $("sourceSel").value=s.source;
    $("gainSel").value=String(s.userGain);
    $("biasToggle").checked=!!s.bias;
    lsbUv=s.lsbUv;
  }catch(e){}
}

(async()=>{
  await loadMap();
  await syncControlsFromDevice();
  await status();
  $("scaleMode").dispatchEvent(new Event("change"));
})();
</script>
</body>
</html>
)IMBCIHTML";

// ============================================================
//  HTTP HELPERS / ENDPOINTS
// ============================================================

static void sendJson(const String &body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

static void handleStatus() {
  String s;
  s.reserve(512);
  s += "{";
  s += "\"adsId\":" + String(adsId) + ",";
  s += "\"source\":\"" + String(sourceName(sourceMode)) + "\",";
  s += "\"userGain\":" + String(userGain) + ",";
  s += "\"gain\":" + String(activeGain()) + ",";
  s += "\"bias\":" + String(biasEnabled ? "true" : "false") + ",";
  s += "\"lsbUv\":" + String(currentLsbUv(), 9) + ",";
  s += "\"samples\":" + String(totalSamples) + ",";
  s += "\"lost\":" + String(lostDrdyEvents) + ",";
  s += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  s += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  s += "\"otaHost\":\"" + String(OTA_HOSTNAME) + ".local\"";
  s += "}";
  sendJson(s);
}

static void handleConfig() {
  if (server.hasArg("source")) {
    String v = server.arg("source");
    if      (v == "physical") sourceMode = SRC_PHYSICAL;
    else if (v == "test")     sourceMode = SRC_TEST;
    else                      sourceMode = SRC_SHORT;
  }

  if (server.hasArg("gain")) {
    uint8_t g = (uint8_t)server.arg("gain").toInt();
    if (g == 1 || g == 2 || g == 4 || g == 6 || g == 8 || g == 12 || g == 24) userGain = g;
  }

  if (server.hasArg("bias")) biasEnabled = (server.arg("bias") == "1");

  adsApplyConfig();
  handleStatus();
}

static void handleMapGet() {
  String s = "{\"labels\":[";
  for (int i = 0; i < 8; ++i) {
    if (i) s += ",";
    String v = getChannelLabel(i);
    v.replace("\\", "\\\\");
    v.replace("\"", "\\\"");
    s += "\"" + v + "\"";
  }
  s += "]}";
  sendJson(s);
}

static void handleMapPost() {
  for (int i = 0; i < 8; ++i) {
    String arg = "ch" + String(i + 1);
    if (!server.hasArg(arg)) continue;
    String value = server.arg(arg);
    value.trim();
    if (value.length() > 16) value = value.substring(0, 16);
    if (value.length() == 0) value = defaultLabels[i];
    char key[5];
    snprintf(key, sizeof(key), "ch%d", i + 1);
    prefs.putString(key, value);
  }
  handleMapGet();
}

static void handleData() {
  uint32_t after = 0;
  uint16_t maxItems = 32;

  if (server.hasArg("after")) after = (uint32_t)strtoul(server.arg("after").c_str(), nullptr, 10);
  if (server.hasArg("max")) {
    int m = server.arg("max").toInt();
    if (m < 1) m = 1;
    if (m > 40) m = 40;
    maxItems = (uint16_t)m;
  }

  Sample batch[40];
  uint16_t n = 0;

  portENTER_CRITICAL(&ringMux);
  uint16_t count = ringCount;
  uint16_t oldest = (ringWrite + RING_SIZE - count) % RING_SIZE;

  for (uint16_t i = 0; i < count && n < maxItems; ++i) {
    Sample &s = ringBuffer[(oldest + i) % RING_SIZE];
    if (s.seq > after) batch[n++] = s;
  }
  portEXIT_CRITICAL(&ringMux);

  String out;
  out.reserve(7000);
  out += "{\"lsbUv\":";
  out += String(currentLsbUv(), 9);
  out += ",\"samples\":[";

  for (uint16_t i = 0; i < n; ++i) {
    if (i) out += ",";
    out += "[";
    out += String(batch[i].seq);
    out += ",";
    out += String(batch[i].tUs);
    for (int ch = 0; ch < 8; ++ch) {
      out += ",";
      out += String(batch[i].ch[ch]);
    }
    out += "]";
  }
  out += "]}";

  sendJson(out);
}

static void handleOtaUpload() {
  HTTPUpload &upload = server.upload();

  // The upload callback runs before the final POST handler, so authentication
  // must be checked here before any firmware bytes are accepted.
  if (!webOtaAuthorized()) return;

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Start: %s\n", upload.filename.c_str());
    stopAcquisitionForOta();

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      resumeAcquisitionAfterOtaError();
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[OTA] Success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    Serial.println("[OTA] Aborted");
    resumeAcquisitionAfterOtaError();
  }
}

// ============================================================
//  SETUP / LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("======================================");
  Serial.println(" IM-BCI 8 — Firmware V1.3.1");
  Serial.println("======================================");

  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_START, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DRDY, INPUT);

  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_START, LOW);
  digitalWrite(PIN_RESET, HIGH);

  SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);

  adsMutex = xSemaphoreCreateMutex();
  prefs.begin("imbci8", false);

  adsHardwareReset();

  // ADS1299 wakes in RDATAC; stop it before reading/writing registers.
  adsCommand(CMD_SDATAC);
  adsId = adsReadReg(REG_ID);
  Serial.printf("[ADS1299] ID = 0x%02X\n", adsId);

  adsApplyConfig();

  // Acquisition task: high priority enough to service 250 Hz reliably.
  xTaskCreatePinnedToCore(
    acquisitionTask,
    "ADS1299 acquisition",
    4096,
    nullptr,
    3,
    &acquisitionTaskHandle,
    1
  );

  attachInterrupt(digitalPinToInterrupt(PIN_DRDY), drdyISR, FALLING);

  // Wi-Fi AP
  WiFi.mode(WIFI_AP);
  delay(100);
  bool apOk = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WiFi] SoftAP: %s\n", apOk ? "OK" : "FAILED");
  Serial.printf("[WiFi] SSID: %s\n", AP_SSID);
  Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

  // ArduinoOTA owns mDNS and also advertises the Arduino IDE network upload service.
  setupArduinoOta();
  Serial.printf("[HTTP] http://%s.local or http://%s\n",
                OTA_HOSTNAME, WiFi.softAPIP().toString().c_str());

  // Web routes
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/config", HTTP_POST, handleConfig);
  server.on("/api/map", HTTP_GET, handleMapGet);
  server.on("/api/map", HTTP_POST, handleMapPost);
  server.on("/api/data", HTTP_GET, handleData);

  server.on("/update", HTTP_POST,
    []() {
      if (!webOtaAuthorized()) {
        server.requestAuthentication();
        return;
      }

      server.sendHeader("Connection", "close");
      if (Update.hasError()) {
        resumeAcquisitionAfterOtaError();
        server.send(500, "text/plain", "OTA FAILED");
      } else {
        server.send(200, "text/plain", "OTA OK — reboot...");
        delay(500);
        ESP.restart();
      }
    },
    handleOtaUpload
  );

  server.on("/favicon.ico", HTTP_GET, []() { server.send(204, "text/plain", ""); });
  server.onNotFound([]() { server.send(404, "text/plain", "IM-BCI 8: not found"); });

  server.begin();
  Serial.println("[HTTP] Server started");
  Serial.println("[BOOT] Default source = INTERNAL SHORT");
  Serial.println();
}

void loop() {
  // Handles Arduino IDE network uploads (espota.py).
  ArduinoOTA.handle();

  // Keep the web UI / fallback web OTA alive as well.
  server.handleClient();

  static uint32_t lastPrint = 0;
  if (!arduinoOtaInProgress && millis() - lastPrint > 3000) {
    lastPrint = millis();
    Serial.printf(
      "[STATUS] ADS=0x%02X source=%s gain=x%u samples=%lu lost=%lu heap=%u\n",
      adsId,
      sourceName(sourceMode),
      activeGain(),
      (unsigned long)totalSamples,
      (unsigned long)lostDrdyEvents,
      ESP.getFreeHeap()
    );
  }

  delay(1);
}
