#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// BTHome & BLE Settings
// ============================================================================
#define DEVICE_NAME "BTHome-ESP32"
#define BTHOME_ADV_INTERVAL_MS 3000 // Regular advertisement interval in ms
#define BTHOME_FAST_ADV_INTERVAL_MS                                            \
  500 // Fast adv interval (e.g. on event trigger)

// ============================================================================
// Device Configuration Structures
// ============================================================================

struct DoorSensorConfig {
  const char *name;    // Descriptive name
  uint8_t pin;         // Digital input pin
  bool activeLow;      // true if LOW reading = Door OPEN
  bool usePullup;      // true to enable internal pullup resistor
  uint32_t debounceMs; // Debounce filter in ms
};

struct LightConfig {
  const char *name;  // Descriptive name
  uint8_t relayPin;  // Digital output pin for light/relay
  bool activeHigh;   // true if HIGH pin = Light ON
  int8_t buttonPin;  // Optional physical toggle button pin (-1 if disabled)
  bool buttonPullup; // Internal pullup for physical button
};

struct NfcConfig {
  bool enabled;            // Enable PN532 NFC reader module
  int8_t sdaPin;           // I2C SDA Pin
  int8_t sclPin;           // I2C SCL Pin
  int8_t resetPin;         // PN532 Hardware Reset Pin (-1 if unconnected)
  int8_t irqPin;           // PN532 Hardware IRQ Pin (-1 if unconnected)
  uint32_t pollIntervalMs; // NFC tag detection polling interval in ms
};

// ============================================================================
// Default Hardware Pin Mappings
// Add/remove entries to support an arbitrary number of Doors or Lights!
// ============================================================================

// 1. Door Sensors (Inputs) - Empty list (no door sensors)
const DoorSensorConfig DOOR_SENSOR_CONFIGS[] = {};

// 2. Lights / Relays (Outputs) - Empty list (no lights)
const LightConfig LIGHT_CONFIGS[] = {};

// 3. Single PN532 NFC Module (I2C only: SDA G38, SCL G39, no RST/IRQ)
const NfcConfig NFC_MODULE_CONFIG = {
    true, // enabled
    38,   // SDA Pin (GPIO 38)
    39,   // SCL Pin (GPIO 39)
    -1,   // Reset Pin (-1: unconnected / I2C only)
    -1,   // IRQ Pin (-1: unconnected / I2C only)
    200   // Polling interval (200 ms)
};

// 4. Status Indicator LED
#define PIN_STATUS_LED -1 // Disabled (-1: no lights/status LED)

#endif // CONFIG_H
