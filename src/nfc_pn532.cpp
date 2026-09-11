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

    // 1. Wake up PN532 from Hard Power Down if Reset pin is configured
    if (m_config.resetPin >= 0) {
        pinMode(m_config.resetPin, OUTPUT);
        digitalWrite(m_config.resetPin, HIGH);
        delay(10);
        digitalWrite(m_config.resetPin, LOW);
        delay(50);
        digitalWrite(m_config.resetPin, HIGH);
        delay(50);
    }

    struct PinPair { int sda; int scl; };
    PinPair candidates[] = {
        {m_config.sdaPin, m_config.sclPin},
        {38, 39},
        {39, 38},
        {2, 1},
        {1, 2},
        {5, 6},
        {6, 5},
        {7, 8},
        {8, 7}
    };

    bool found = false;
    int activeSda = -1;
    int activeScl = -1;

    for (const auto& pair : candidates) {
        if (pair.sda < 0 || pair.scl < 0) continue;

        Wire.end(); // Reset I2C bus state
        pinMode(pair.sda, INPUT_PULLUP);
        pinMode(pair.scl, INPUT_PULLUP);
        Wire.begin(pair.sda, pair.scl);
        Wire.setClock(100000);
        Wire.setTimeOut(1000);

        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("[I2C Scan] Device RESPONDED at address 0x%02X on SDA Pin %d, SCL Pin %d!\n", addr, pair.sda, pair.scl);
                found = true;
                activeSda = pair.sda;
                activeScl = pair.scl;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        Serial.println("[PN532] Tested all pin pairs (38/39, 39/38, 2/1, 1/2, 5/6, 7/8). No I2C device responded.");
        Serial.println("[PN532] Try toggling DIP Switch 1 (set to OFF) & Switch 2 (set to ON).");
        m_initialized = false;
        return false;
    }
    
    m_nfc.begin();
    // Re-apply custom pins after m_nfc.begin() overrides Wire
    if (activeSda >= 0 && activeScl >= 0) {
        Wire.begin(activeSda, activeScl);
        Wire.setClock(100000);
        Wire.setTimeOut(1000);
    }

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
    m_nfc.setPassiveActivationRetries(0xFF); // Retry 255 times (~50ms) to energize tag antenna

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

    // Attempt to detect passive ISO14443A tag (Mifare cards / tags) with 100ms timeout
    bool success = m_nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

    static uint32_t lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        Serial.println("[PN532] Reader active & listening for 13.56MHz NFC tags/cards...");
    }

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
