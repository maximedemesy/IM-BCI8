#pragma once

// Copy this file to "secrets.h" in the same firmware directory.
// Never commit your real secrets.h to a public repository.

// ESP32 SoftAP password: minimum 8 characters.
#define IMBCI_AP_PASSWORD "CHANGE_ME_WIFI"

// Password used by both ArduinoOTA and the authenticated web OTA endpoint.
#define IMBCI_OTA_PASSWORD "CHANGE_ME_OTA"
