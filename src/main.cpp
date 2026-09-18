#include <Arduino.h>
#include <vector>
#include <NimBLEDevice.h>

#include "config.h"
#include "bthome_encoder.h"
#include "door_sensor.h"

#if ENABLE_PN532_NFC
#include "nfc_manager.h"
NfcManager* g_nfcManager = nullptr;
#endif

// BTHome Service UUID (0xFCD2)
static const uint16_t BTHOME_SERVICE_UUID = 0xFCD2;

// Calculate dynamic door sensor count from config array
static const size_t NUM_DOOR_SENSORS = sizeof(DOOR_SENSOR_CONFIGS) / sizeof(DoorSensorConfig);

// Dynamic door sensor instances
std::vector<DoorSensor*> g_doorSensors;

// BLE Advertising state globals
NimBLEAdvertising* pAdvertising = nullptr;
uint8_t g_packetId = 0;
uint32_t g_lastAdvUpdate = 0;
uint32_t g_currentAdvInterval = BTHOME_ADV_INTERVAL_MS;

void updateBTHomeAdvertising(bool forceFast = false) {
    if (!pAdvertising) return;

    BTHomeEncoder encoder;
    
    // 1. Add Packet Sequence ID (0x00)
    encoder.addPacketId(g_packetId++);

    // 2. Add state of all Door Sensors (0x1A)
    for (size_t i = 0; i < g_doorSensors.size(); ++i) {
        if (!encoder.addDoorState(g_doorSensors[i]->isOpen())) {
            Serial.printf("[BLE] Warning: BTHome payload full, skipped Door #%d\n", i + 1);
            break;
        }
    }

#if ENABLE_PN532_NFC
    // 3. Add Button Event (0x3A) and Tag UID (0x3E) if tag present
    // NOTE: Added strictly in ascending Object ID order (0x00 < 0x1A < 0x3A < 0x3E)
    if (g_nfcManager && g_nfcManager->isInitialized() && g_nfcManager->isTagPresent()) {
        encoder.addButtonEvent(0x01); // Button press event (0x01) on NFC tap

        const auto& tag = g_nfcManager->getLastTag();
        uint32_t uid32 = 0;
        for (size_t k = 0; k < tag.uid.size() && k < 4; ++k) {
            uid32 = (uid32 << 8) | tag.uid[k];
        }
        encoder.addTagUid(uid32); // 4-byte uint32 count/ID (0x3E)
    }
#endif

    // Build NimBLE Service Data
    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);

    std::string serviceDataStr;
    const uint8_t* payload = encoder.getPayload();
    size_t payloadLen = encoder.getPayloadLength();
    
    serviceDataStr.append((const char*)payload, payloadLen);
    advData.setServiceData(NimBLEUUID(BTHOME_SERVICE_UUID), serviceDataStr);
    
    // Scan Response Data containing Device Name
    NimBLEAdvertisementData scanData;
    scanData.setName(DEVICE_NAME);

    pAdvertising->stop();
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanData);
    pAdvertising->start();

    g_lastAdvUpdate = millis();
    g_currentAdvInterval = forceFast ? BTHOME_FAST_ADV_INTERVAL_MS : BTHOME_ADV_INTERVAL_MS;

    Serial.printf("[BLE Adv] Packet #%u | Length %zu bytes: ", g_packetId - 1, payloadLen);
    for (size_t k = 0; k < payloadLen; ++k) {
        Serial.printf("%02X ", payload[k]);
    }
    Serial.println();

#if PIN_STATUS_LED >= 0
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(10);
    digitalWrite(PIN_STATUS_LED, LOW);
#endif
}

void setup() {
    Serial.begin(115200);
    unsigned long startTime = millis();
    while (!Serial && (millis() - startTime < 2000));
    delay(500);
    Serial.println("\n==========================================");
    Serial.println("   Initializing ESP32 BTHome Node");
    Serial.println("==========================================");

#if PIN_STATUS_LED >= 0
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
#endif

    // 1. Initialize Door Sensors
    Serial.printf("[Init] Door Sensors Count: %u\n", NUM_DOOR_SENSORS);
    for (size_t i = 0; i < NUM_DOOR_SENSORS; ++i) {
        DoorSensor* ds = new DoorSensor(DOOR_SENSOR_CONFIGS[i]);
        ds->begin();
        g_doorSensors.push_back(ds);
        Serial.printf("  - Door #%u [%s] on Pin %u -> State: %s\n",
                      i + 1, ds->getName(), ds->getPin(),
                      ds->isOpen() ? "OPEN" : "CLOSED");
    }

#if ENABLE_PN532_NFC
    // 2. Initialize Optional PN532 NFC Manager
    Serial.printf("[Init] Initializing PN532 NFC Module (I2C SDA: %d, SCL: %d)\n",
                  PN532_I2C_SDA, PN532_I2C_SCL);
    g_nfcManager = new NfcManager(PN532_I2C_SDA, PN532_I2C_SCL, I2C_NUM_0, PN532_RESET_PIN);
    if (!g_nfcManager->begin()) {
        Serial.println("[Init] Note: PN532 NFC module not present or disabled.");
    } else {
        Serial.println("[Init] PN532 NFC Module Initialized Successfully.");
    }
#else
    Serial.println("[Init] PN532 NFC Module Disabled in config.h.");
#endif

    // 3. Initialize NimBLE Advertising
    Serial.println("[Init] Starting NimBLE BLE Stack: " DEVICE_NAME);
    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9dBm Max TX Power

    pAdvertising = NimBLEDevice::getAdvertising();

    // Broadcast initial states
    updateBTHomeAdvertising(true);
    Serial.println("[Ready] ESP32 BTHome Node is Active & Broadcasting!\n");
}

void loop() {
    bool stateChanged = false;

    // 1. Poll all Door Sensors
    for (size_t i = 0; i < g_doorSensors.size(); ++i) {
        if (g_doorSensors[i]->update()) {
            Serial.printf("[Event] Door #%u [%s] -> %s\n",
                          i + 1, g_doorSensors[i]->getName(),
                          g_doorSensors[i]->isOpen() ? "OPEN" : "CLOSED");
            stateChanged = true;
        }
    }

#if ENABLE_PN532_NFC
    // 2. Poll Optional PN532 NFC Reader
    if (g_nfcManager && g_nfcManager->isInitialized() && g_nfcManager->update()) {
        if (g_nfcManager->isTagPresent()) {
            Serial.printf("[Event] NFC Tag Tapped! Type: %d, UID/IDm: %s\n",
                          static_cast<int>(g_nfcManager->getLastTag().type),
                          g_nfcManager->getTagUidString().c_str());
        } else {
            Serial.println("[Event] NFC Tag Removed.");
        }
        stateChanged = true;
    }
#endif

    // 3. Update BLE Advertisement if state changed or interval elapsed
    if (stateChanged) {
        updateBTHomeAdvertising(true);
    } else if (millis() - g_lastAdvUpdate >= g_currentAdvInterval) {
        updateBTHomeAdvertising(false);
    }

    delay(10);
}
