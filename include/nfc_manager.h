#ifndef NFC_MANAGER_H
#define NFC_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <string>
#include "pn532_cxx/pn532.hpp"
#include "pn532_hal/i2c.hpp"

enum class NfcTagType {
    NONE,
    MIFARE_CLASSIC,
    ISO14443_4_PHONE,
    FELICA
};

struct NfcTagResult {
    NfcTagType type = NfcTagType::NONE;
    std::vector<uint8_t> uid;
    std::string uidStr;
};

class NfcManager {
public:
    NfcManager(uint8_t sdaPin, uint8_t sclPin, i2c_port_t port = I2C_NUM_0, int8_t resetPin = -1);
    ~NfcManager();

    bool begin();
    bool update(); // Returns true if tag state changed (tag tapped or removed)

    bool isInitialized() const { return _initialized; }
    bool isTagPresent() const { return _tagPresent; }
    const NfcTagResult& getLastTag() const { return _lastTag; }
    std::string getTagUidString() const { return _lastTag.uidStr; }

private:
    uint8_t _sdaPin;
    uint8_t _sclPin;
    i2c_port_t _port;
    int8_t _resetPin;
    bool _initialized = false;

    pn532::I2cTransport* _transport = nullptr;
    pn532::Frontend* _nfc = nullptr;

    bool _tagPresent = false;
    NfcTagResult _lastTag;
    uint32_t _lastScanTime = 0;

    bool scanMifareAndIsoDep(NfcTagResult& result);
    bool scanFelica(NfcTagResult& result);
};

#endif // NFC_MANAGER_H
