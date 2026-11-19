# IM-BCI8 Firmware

This directory contains the firmware used by the IM-BCI8 Rev.A hardware.

The current reference firmware is developed for the **ESP32-S3-WROOM-1-N8R8** and controls:

- ADS1299 configuration and acquisition;
- 8-channel EEG sampling;
- Wi-Fi communication;
- the embedded browser interface;
- signal visualization;
- basic spectral analysis;
- CSV recording;
- experimental markers;
- hardware configuration;
- firmware updates.

> **Current reference version:** V1.3  
> **Hardware:** IM-BCI8 Rev.A  
> **Framework:** Arduino / ESP32  
> **Status:** Experimental, actively developed

The firmware is part of an experimental EEG / BCI platform and is **not medical software**.

See [`../SAFETY.md`](../SAFETY.md) before using the device for human EEG acquisition.

---

## 1. Firmware structure

```text
firmware/
├── README.md
│
└── IM_BCI8_V1_3/
    ├── IM_BCI8_V1_3.ino
    └── secrets.example.h
```

The current reference firmware is intentionally kept close to the version validated on the Rev.A prototype.

A more modular source architecture may be introduced in future releases, but major restructuring should be followed by renewed hardware testing.

---

## 2. Main features

The current firmware provides:

### ADS1299 acquisition

- 8 simultaneous channels;
- 250 samples per second;
- configurable gain;
- physical input acquisition;
- ADS1299 internal short mode;
- ADS1299 internal test-signal mode;
- common SRB1 reference support;
- configurable BIAS;
- DRDY-synchronized sampling;
- SPI communication with the ADS1299.

### Wireless operation

The ESP32-S3 creates its own Wi-Fi access point.

A computer, tablet or smartphone can connect directly to IM-BCI8 without an external router or acquisition dongle.

The browser interface is then available locally from the ESP32.

### Browser interface

The current embedded interface includes:

- eight separate live EEG plots;
- channel identification;
- electrode-position mapping;
- common or individual signal scaling;
- RMS measurements;
- peak-to-peak measurements;
- out-of-range indication;
- FFT / frequency-spectrum visualization;
- Delta band visualization;
- Theta band visualization;
- Alpha band visualization;
- Beta band visualization;
- acquisition-mode configuration;
- ADS1299 test modes;
- experiment markers;
- CSV recording;
- system information;
- firmware update interface.

The browser interface is served directly by the ESP32-S3.

No dedicated desktop application is required for basic acquisition and visualization.

---

## 3. Hardware pin mapping

The Rev.A ADS1299 interface uses:

| Function | ESP32-S3 GPIO |
| --- | ---: |
| START | GPIO8 |
| RESET | GPIO9 |
| CS | GPIO10 |
| MOSI / ADS DIN | GPIO11 |
| SCLK | GPIO12 |
| MISO / ADS DOUT | GPIO13 |
| DRDY | GPIO14 |
| USB D- | GPIO19 |
| USB D+ | GPIO20 |
| Status LED | GPIO4 |

These assignments correspond to the published Rev.A PCB.

Changing them in firmware without matching hardware changes will prevent correct operation.

---

## 4. SPI configuration

The current reference firmware communicates with the ADS1299 using:

```text
SPI frequency : 1 MHz
SPI mode      : MODE1
```

The deliberately moderate SPI frequency was selected for reliable bring-up and acquisition rather than maximum possible bus speed.

---

## 5. Acquisition architecture

ADS1299 sampling is synchronized using the `DRDY` signal.

The firmware uses an interrupt-driven acquisition architecture rather than polling the ADS1299 continuously from the web-interface code.

Conceptually:

```text
ADS1299
   │
 DRDY
   │
   ▼
Interrupt
   │
   ▼
Acquisition task
   │
   ▼
Sample buffer
   │
   ├── Web interface
   ├── Visualization
   ├── Spectrum analysis
   └── CSV recording
```

This separation helps prevent web-interface activity and Wi-Fi communication from directly controlling the sampling timing.

---

## 6. Signal conversion

The ADS1299 provides signed 24-bit samples.

The firmware converts raw ADC counts into voltage using the configured ADS1299 gain and the reference-voltage value used by the Rev.A prototype.

The current firmware uses approximately:

```text
VREF ≈ 4.46 V
```

for its ADC-to-voltage conversion.

If the analog hardware, ADS1299 reference configuration or gain settings are modified, the conversion should be reviewed.

---

## 7. Browser-side processing

Some processing is intentionally performed for visualization rather than altering the underlying hardware acquisition.

The current interface includes:

- display mean removal;
- optional 50 Hz notch filtering;
- auto-scaling;
- FFT-based frequency visualization;
- EEG-band visualization.

These processing functions are intended primarily for interactive experimentation and visualization.

They should not automatically be treated as validated scientific signal-processing pipelines.

Raw or minimally processed data should be preferred when performing independent offline analysis.

---

## 8. Frequency spectrum

The current spectrum display uses a finite data window to estimate signal content in the EEG frequency range.

The interface displays activity in approximately:

```text
Delta : 0.5 – 4 Hz
Theta : 4   – 8 Hz
Alpha : 8   – 12 Hz
Beta  : 13  – 30 Hz
```

These bands are intended as convenient visualization ranges and should not be interpreted as diagnostic classifications.

---

## 9. Current electrode mapping

The default development mapping is:

```text
CH1 → Fp1
CH2 → Fp2
CH3 → C3
CH4 → C4
CH5 → P7
CH6 → P8
CH7 → O1
CH8 → O2
```

with separate:

```text
REF
BIAS
```

electrodes.

This mapping is configurable and is not imposed by the PCB.

---

## 10. Local web API

The current firmware exposes local HTTP endpoints used by the browser interface.

Current endpoints include:

```text
/api/status
/api/config
/api/map
/api/data
/update
```

These interfaces are currently intended primarily for the embedded IM-BCI8 browser application.

They should not yet be considered a stable external API.

Endpoint names, payload formats and behavior may change during development.

---

## 11. Arduino IDE configuration

The tested development configuration uses an ESP32-S3 Arduino environment with settings equivalent to:

```text
Board: ESP32S3 Dev Module

USB CDC On Boot: Enabled
USB Mode: Hardware CDC and JTAG
Upload Mode: UART0 / Hardware CDC

CPU Frequency: 240 MHz
Flash Mode: QIO 80 MHz
Upload Speed: 460800

PSRAM: QSPI PSRAM
```

The Rev.A board uses an:

```text
ESP32-S3-WROOM-1-N8R8
```

module.

Exact Arduino menu wording may vary depending on the installed ESP32 Arduino core version.

---

## 12. First USB flash

The initial firmware installation should be performed **without a person connected to the electrode system**.

Recommended procedure:

```text
1. Disconnect all electrodes from any person
2. Connect IM-BCI8 by USB
3. Open IM_BCI8_V1_3.ino
4. Select the ESP32-S3 configuration
5. Compile
6. Upload
7. Open the Serial Monitor
8. Verify ESP32 startup
9. Verify ADS1299 detection
10. Verify internal test modes
11. Disconnect USB
12. Switch to battery operation before human EEG use
```

The expected ADS1299 device identification during correct communication is:

```text
0x3E
```

---

## 13. Wi-Fi access

The current firmware operates as a local Wi-Fi access point.

The default development SSID is:

```text
IM-BCI-8
```

Once connected, the web interface is normally reachable at:

```text
http://192.168.4.1
```

Local hostname support may also be available depending on the client operating system and network configuration.

The default network configuration is intended for local experimental use.

It should not be interpreted as a hardened network-security architecture.

---

## 14. Firmware updates

The current firmware supports local firmware updates.

Depending on the development workflow, firmware can be updated using:

- the ESP32 development environment;
- local OTA update support;
- the embedded browser update interface.

OTA is intended primarily to avoid repeatedly opening the electronics enclosure after initial development.

### Important

Do not perform firmware updates while electrodes are attached to a person.

For USB flashing:

**disconnect the headset from the user first.**

---

## 15. Secrets and credentials

Reusable private credentials must **not** be committed to this repository.

Local credentials should be placed in:

```text
secrets.h
```

which should be excluded from Git.

A template may be provided as:

```text
secrets.example.h
```

For example:

```cpp
#pragma once

#define IMBCI_OTA_PASSWORD "CHANGE_ME"
```

The actual local file can then contain the user's chosen credential without becoming part of the public repository.

The repository `.gitignore` should contain:

```gitignore
**/secrets.h
```

Do not commit:

- personal Wi-Fi credentials;
- reusable OTA passwords;
- API keys;
- private network credentials;
- personal development credentials.

Public default configuration values should be clearly distinguished from private credentials.

---

## 16. Internal test modes

Before testing real electrodes, the ADS1299 internal modes should be used to validate the digital and analog acquisition chain.

### Internal short

The internal-short configuration is useful for checking:

- ADS1299 communication;
- conversion stability;
- digital acquisition;
- intrinsic electronic noise.

A successful internal-short test does **not** validate the external electrode path.

### Internal test signal

The ADS1299 internal test generator is useful for checking:

- channel synchronization;
- scaling;
- acquisition continuity;
- browser plotting;
- data transport.

### Physical inputs

Physical-input mode validates the actual external channel connections.

Disconnected physical inputs can show:

- large mains pickup;
- drift;
- saturation;
- movement-sensitive artifacts.

This behavior is expected for high-impedance floating biopotential inputs and should not automatically be interpreted as a board fault.

---

## 17. Recommended validation sequence

For a newly flashed or modified firmware:

```text
1. Boot ESP32
2. Confirm ADS1299 ID
3. Internal short
4. Internal test signal
5. Verify eight channels
6. Verify physical input response
7. Verify REF path
8. Verify BIAS behavior
9. Verify Wi-Fi / browser acquisition
10. Verify CSV output
11. Disconnect USB and test battery-only operation
12. Only then begin human EEG testing
```

Significant firmware changes should repeat this validation sequence.

---

## 18. Human EEG operation

For human EEG acquisition:

```text
Battery     connected
Wi-Fi       enabled

USB         disconnected
charger     disconnected
bench PSU   disconnected
oscilloscope disconnected
function generator disconnected
```

See [`../SAFETY.md`](../SAFETY.md) for the full safety requirements.

Firmware functionality does not replace electrical isolation or hardware safety measures.

---

## 19. Current limitations

The current firmware is an experimental development version.

Known areas for future improvement include:

- more modular source organization;
- improved external API stability;
- improved streaming interfaces;
- BrainFlow integration;
- Lab Streaming Layer integration;
- MNE-compatible workflows;
- more efficient spectrum processing;
- improved recording metadata;
- additional experiment tools;
- improved configuration persistence;
- improved network / OTA security;
- additional battery-related functionality when supported by future hardware.

---

## 20. AI-assisted development

A significant portion of the IM-BCI8 firmware and embedded browser interface was developed using AI-assisted programming.

AI tools were used for:

- code generation;
- debugging;
- architecture discussions;
- UI development;
- signal-processing prototypes;
- documentation.

Generated code was tested on the physical Rev.A prototype rather than assumed to be correct.

This repository intentionally documents that development process rather than presenting the firmware as entirely hand-written.

---

## 21. Contributions

Firmware contributions are welcome.

Particularly useful areas include:

- acquisition robustness;
- ADS1299 driver improvements;
- WebSocket or other streaming support;
- BrainFlow support;
- LSL support;
- MNE workflows;
- browser performance;
- signal-processing improvements;
- recording formats;
- security improvements;
- reproducible EEG experiments.

Changes affecting acquisition timing, ADS1299 configuration or BIAS operation should include a description of how they were validated on hardware.

---

## Versioning

Firmware versions should remain traceable to the hardware revision on which they were tested.

For example:

```text
Hardware Rev.A
└── Firmware V1.3
```

Future releases should avoid silently replacing a previously tested firmware version.

Older tested versions should remain available through Git history or tagged releases whenever practical.
