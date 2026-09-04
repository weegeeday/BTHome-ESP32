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

// 1. Door Sensors (Inputs) - Infinite list supported!
const DoorSensorConfig DOOR_SENSOR_CONFIGS[] = {
    {"Front Door", 19, true, true, 50},  // Pin 19 (Main Door)
    {"Back Door", 23, true, true, 50},   // Pin 23 (Secondary Door)
    {"Garage Door", 15, true, true, 50}, // Pin 15 (Tertiary Door)
};

// 2. Lights / Relays (Outputs) - Infinite list supported!
const LightConfig LIGHT_CONFIGS[] = {
    {"Living Room Light", 18, true, 4, true}, // Relay Pin 18, Button Pin 4
    {"Kitchen Light", 5, true, -1, true},     // Relay Pin 5, No button
};

// 3. Single PN532 NFC Module
const NfcConfig NFC_MODULE_CONFIG = {
    true, // enabled
    21,   // SDA Pin (GPIO 21)
    22,   // SCL Pin (GPIO 22)
    17,   // Reset Pin (GPIO 17)
    16,   // IRQ Pin (GPIO 16)
    200   // Polling interval (200 ms)
};

// 4. Status Indicator LED
#define PIN_STATUS_LED                                                         \
  2 // Built-in LED on standard ESP32 boards (-1 to disable)

#endif // CONFIG_H
