#include "nfc_manager.h"
#include <Arduino.h>
#include <esp_log.h>
#include <driver/i2c.h>
#include <sstream>
#include <iomanip>

static const char *TAG = "NFC_MANAGER";

static std::string bytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (uint8_t b : bytes) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

NfcManager::NfcManager(uint8_t sdaPin, uint8_t sclPin, i2c_port_t port, int8_t resetPin)
    : _sdaPin(sdaPin), _sclPin(sclPin), _port(port), _resetPin(resetPin) {}

NfcManager::~NfcManager() {
    delete _nfc;
    delete _transport;
}

bool NfcManager::begin() {
    _initialized = false;
    Serial.printf("[NFC] Initializing PN532 on I2C Port %d (SDA Pin: %d, SCL Pin: %d)...\n", _port, _sdaPin, _sclPin);

    if (_resetPin >= 0) {
        Serial.printf("[NFC] Resetting PN532 hardware pin %d...\n", _resetPin);
        pinMode(_resetPin, OUTPUT);
        digitalWrite(_resetPin, LOW);
        delay(50);
        digitalWrite(_resetPin, HIGH);
        delay(100);
    }

    _transport = new pn532::I2cTransport(_port, (gpio_num_t)_sdaPin, (gpio_num_t)_sclPin);

    // Scan I2C bus to verify physical hardware connection
    Serial.println("[NFC] Performing I2C Bus Scan...");
    int devicesFound = 0;
    bool foundPN532Address = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(_port, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            Serial.printf("  [I2C Scan] -> Responded at address 0x%02X\n", addr);
            devicesFound++;
            if (addr == 0x24) {
                foundPN532Address = true;
            }
        }
    }

    if (devicesFound == 0) {
        Serial.println("[NFC Error] No I2C devices responded! NFC module will remain disabled.");
        return false;
    } else if (!foundPN532Address) {
        Serial.println("[NFC Warning] I2C devices responded, but 0x24 (PN532 default) was not among them.");
        Serial.println("  Check DIP switches: I2C mode requires SEL0=HIGH (1), SEL1=LOW (0).");
        return false;
    } else {
        Serial.println("[NFC Success] Found PN532 module at address 0x24!");
    }

    _nfc = new pn532::Frontend(*_transport);

    pn532::Status st = _nfc->begin();
    if (st != pn532::Status::SUCCESS) {
        Serial.printf("[NFC Error] PN532 initialization command failed with status: %d\n", static_cast<int>(st));
        return false;
    }

    auto version = _nfc->GetFirmwareVersion();
    if (version.has_value()) {
        Serial.printf("[NFC Success] PN532 Firmware Version: 0x%08X\n", version.value());
    } else {
        Serial.println("[NFC Warning] Could not query PN532 firmware version, continuing...");
    }

    // Set passive retries to 1 so scanning does not block FreeRTOS execution
    _nfc->setPassiveActivationRetries(0x01);
    
    _initialized = true;
    Serial.println("[NFC Ready] PN532 Module Initialized! Ready to scan FeliCa, Mifare, ISO14443 & Phones.\n");
    return true;
}

bool NfcManager::scanMifareAndIsoDep(NfcTagResult& result) {
    if (!_nfc || !_initialized) return false;

    std::vector<uint8_t> uid;
    std::array<uint8_t, 2> sens_res{};
    uint8_t sel_res = 0;

    pn532::Status st = _nfc->InListPassiveTarget(0x00, uid, sens_res, sel_res, 150);
    if (st == pn532::Status::SUCCESS && !uid.empty()) {
        result.uid = uid;
        result.uidStr = bytesToHex(uid);
        if (sel_res & 0x20) {
            result.type = NfcTagType::ISO14443_4_PHONE;
        } else {
            result.type = NfcTagType::MIFARE_CLASSIC;
        }
        return true;
    }
    return false;
}

bool NfcManager::scanFelica(NfcTagResult& result) {
    if (!_nfc || !_initialized) return false;

    std::vector<uint8_t> uid;
    std::array<uint8_t, 2> sens_res{};
    uint8_t sel_res = 0;

    // Baudrate 0x01 = 212 kbps FeliCa / NFC-F
    pn532::Status st = _nfc->InListPassiveTarget(0x01, uid, sens_res, sel_res, 150);
    if (st == pn532::Status::SUCCESS && !uid.empty()) {
        result.uid = uid;
        result.uidStr = bytesToHex(uid);
        result.type = NfcTagType::FELICA;
        return true;
    }
    return false;
}

bool NfcManager::update() {
    if (!_initialized) {
        return false;
    }

    uint32_t now = millis();
    if (now - _lastScanTime < 250) {
        return false;
    }
    _lastScanTime = now;

    NfcTagResult currentResult;
    bool found = scanMifareAndIsoDep(currentResult);
    if (!found) {
        found = scanFelica(currentResult);
    }

    if (found) {
        if (!_tagPresent || currentResult.uidStr != _lastTag.uidStr) {
            _tagPresent = true;
            _lastTag = currentResult;
            Serial.printf("[NFC Event] Tag Detected! Type: %d, UID/IDm: %s\n",
                          static_cast<int>(_lastTag.type), _lastTag.uidStr.c_str());
            return true;
        }
    } else {
        if (_tagPresent) {
            _tagPresent = false;
            _lastTag = NfcTagResult();
            Serial.println("[NFC Event] Tag Removed.");
            return true;
        }
    }

    return false;
}
