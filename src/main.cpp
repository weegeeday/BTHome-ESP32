#include <Arduino.h>
#include <vector>
#include <NimBLEDevice.h>

#include "config.h"
#include "bthome_encoder.h"
#include "door_sensor.h"
#include "light_control.h"
#include "nfc_pn532.h"

// BTHome Service UUID
static const uint16_t BTHOME_SERVICE_UUID = 0xFCD2;

// Calculate dynamic counts from config arrays
static const size_t NUM_DOOR_SENSORS = sizeof(DOOR_SENSOR_CONFIGS) / sizeof(DoorSensorConfig);
static const size_t NUM_LIGHTS       = sizeof(LIGHT_CONFIGS) / sizeof(LightConfig);

// Dynamic instances
std::vector<DoorSensor*> g_doorSensors;
std::vector<LightControl*> g_lights;
NfcPn532 g_nfcModule(NFC_MODULE_CONFIG);

// BLE Advertising state globals
NimBLEAdvertising* pAdvertising = nullptr;
uint8_t g_packetId = 0;
uint32_t g_lastAdvUpdate = 0;
uint32_t g_currentAdvInterval = BTHOME_ADV_INTERVAL_MS;

// NFC Tag State
uint32_t g_lastNfcTagUid = 0;
bool g_nfcTagActive = false;
uint32_t g_nfcTagClearTime = 0;

void updateBTHomeAdvertising(bool forceFast = false) {
    if (!pAdvertising) return;

    BTHomeEncoder encoder;
    
    // Add Packet Sequence ID (0x00) for deduplication
    encoder.addPacketId(g_packetId++);

    // Add state of all Door Sensors (0x1A)
    for (size_t i = 0; i < g_doorSensors.size(); ++i) {
        if (!encoder.addDoorState(g_doorSensors[i]->isOpen())) {
            Serial.printf("[BLE] Warning: BTHome payload full, skipped Door #%d\n", i + 1);
            break;
        }
    }

    // Add state of all Lights (0x10)
    for (size_t i = 0; i < g_lights.size(); ++i) {
        if (!encoder.addLightState(g_lights[i]->isOn())) {
            Serial.printf("[BLE] Warning: BTHome payload full, skipped Light #%d\n", i + 1);
            break;
        }
    }

    // Add NFC tag event if active (0x3A & 0x0C)
    if (g_nfcTagActive) {
        encoder.addButtonEvent(0x01); // 0x01 = Button press / RFID Tag event
        if (g_lastNfcTagUid != 0) {
            encoder.addTagUid(g_lastNfcTagUid);
        }
    }

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

#if PIN_STATUS_LED >= 0
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(10);
    digitalWrite(PIN_STATUS_LED, LOW);
#endif
}

void setup() {
    Serial.begin(115200);
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

    // 2. Initialize Light Controls
    Serial.printf("[Init] Light Controls Count: %u\n", NUM_LIGHTS);
    for (size_t i = 0; i < NUM_LIGHTS; ++i) {
        LightControl* lc = new LightControl(LIGHT_CONFIGS[i]);
        lc->begin();
        g_lights.push_back(lc);
        Serial.printf("  - Light #%u [%s] Relay Pin %u -> State: %s\n",
                      i + 1, lc->getName(), lc->getRelayPin(),
                      lc->isOn() ? "ON" : "OFF");
    }

    // 3. Initialize PN532 NFC Reader
    if (g_nfcModule.isEnabled()) {
        Serial.println("[Init] PN532 NFC Module enabled");
        if (!g_nfcModule.begin()) {
            Serial.println("[PN532] Warning: NFC Module initialization failed or not detected");
        }
    } else {
        Serial.println("[Init] PN532 NFC Module disabled in config");
    }

    // 4. Initialize NimBLE Advertising
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

    // 2. Poll all Light Controls & Physical Buttons
    for (size_t i = 0; i < g_lights.size(); ++i) {
        if (g_lights[i]->update()) {
            Serial.printf("[Event] Light #%u [%s] -> %s\n",
                          i + 1, g_lights[i]->getName(),
                          g_lights[i]->isOn() ? "ON" : "OFF");
            stateChanged = true;
        }
    }

    // 3. Poll PN532 NFC Module
    if (g_nfcModule.isEnabled()) {
        uint32_t scannedUid = 0;
        uint8_t uidBytes[7] = {0};
        uint8_t uidLen = 0;

        if (g_nfcModule.pollTag(scannedUid, uidBytes, uidLen)) {
            Serial.printf("[Event] PN532 NFC Tag Scanned! Hex UID: 0x%08X\n", scannedUid);
            g_lastNfcTagUid = scannedUid;
            g_nfcTagActive = true;
            g_nfcTagClearTime = millis() + 3000; // Keep tag event active for 3s
            stateChanged = true;
        }

        if (g_nfcTagActive && millis() > g_nfcTagClearTime) {
            g_nfcTagActive = false;
            g_lastNfcTagUid = 0;
            stateChanged = true;
        }
    }

    // 4. Update BLE Advertisement if state changed or interval elapsed
    if (stateChanged) {
        updateBTHomeAdvertising(true);
    } else if (millis() - g_lastAdvUpdate >= g_currentAdvInterval) {
        updateBTHomeAdvertising(false);
    }

    delay(10);
}
