#include "nfc_pn532.h"

NfcPn532::NfcPn532(const NfcConfig& config)
    : m_config(config),
      m_nfc(config.irqPin >= 0 ? config.irqPin : 255, config.resetPin >= 0 ? config.resetPin : 255),
      m_initialized(false),
      m_lastPollTime(0),
      m_lastDetectedUid(0),
      m_lastDetectionTime(0) {}

bool NfcPn532::isEnabled() const {
    return m_config.enabled;
}

bool NfcPn532::begin() {
    if (!m_config.enabled) {
        return false;
    }

    if (m_config.sdaPin >= 0 && m_config.sclPin >= 0) {
        Wire.begin(m_config.sdaPin, m_config.sclPin);
    } else {
        Wire.begin();
    }
    
    m_nfc.begin();

    uint32_t versiondata = m_nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[PN532] Warning: Didn't find PN532 board via I2C!");
        m_initialized = false;
        return false;
    }

    Serial.printf("[PN532] Found chip PN5%02X, Firmware ver. %d.%d\n",
                  (versiondata >> 24) & 0xFF,
                  (versiondata >> 16) & 0xFF,
                  (versiondata >> 8) & 0xFF);

    // Configure board to read RFID tags
    m_nfc.SAMConfig();
    m_nfc.setPassiveActivationRetries(0x01); // Quick non-blocking read retry count

    m_initialized = true;
    return true;
}

bool NfcPn532::isReady() const {
    return m_initialized;
}

bool NfcPn532::pollTag(uint32_t &outUid32, uint8_t *outUidBytes, uint8_t &outUidLen) {
    if (!m_config.enabled || !m_initialized) {
        return false;
    }

    if (millis() - m_lastPollTime < m_config.pollIntervalMs) {
        return false;
    }
    m_lastPollTime = millis();

    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    // Attempt to detect passive ISO14443A tag (Mifare cards / tags) with 30ms non-blocking timeout
    bool success = m_nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 30);

    if (success && uidLength > 0) {
        uint32_t currentUid32 = 0;
        for (uint8_t i = 0; i < min((uint8_t)4, uidLength); i++) {
            currentUid32 |= ((uint32_t)uid[i]) << (i * 8);
        }

        // Debounce same tag reading within 1.5 seconds
        if (currentUid32 == m_lastDetectedUid && (millis() - m_lastDetectionTime < 1500)) {
            return false;
        }

        m_lastDetectedUid = currentUid32;
        m_lastDetectionTime = millis();

        outUid32 = currentUid32;
        outUidLen = uidLength;
        if (outUidBytes) {
            memcpy(outUidBytes, uid, uidLength);
        }

        Serial.printf("[PN532] Tag Scanned! UID Len: %d, Hex UID: 0x%08X\n", uidLength, currentUid32);
        return true;
    }

    return false;
}
