#ifndef NFC_PN532_H
#define NFC_PN532_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "config.h"

class NfcPn532 {
public:
    NfcPn532(const NfcConfig& config);

    bool begin();

    // Call periodically in loop. Returns true if a new NFC tag/card was detected.
    bool pollTag(uint32_t &outUid32, uint8_t *outUidBytes, uint8_t &outUidLen);

    bool isReady() const;
    bool isEnabled() const;

private:
    NfcConfig m_config;
    Adafruit_PN532 m_nfc;
    bool m_initialized;
    uint32_t m_lastPollTime;
    uint32_t m_lastDetectedUid;
    uint32_t m_lastDetectionTime;
};

#endif // NFC_PN532_H
