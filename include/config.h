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
// PN532 NFC Module Settings (I2C) - Optional
// ============================================================================
#define ENABLE_PN532_NFC true // Set to false to completely disable NFC
#define PN532_I2C_SDA 38      // I2C SDA Pin
#define PN532_I2C_SCL 39      // I2C SCL Pin
#define PN532_RESET_PIN -1    // Hardware reset pin (-1 if not used)

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

// ============================================================================
// Door Sensors (Inputs) - Configurable array
// ============================================================================
const DoorSensorConfig DOOR_SENSOR_CONFIGS[] = {};

// Status Indicator LED (-1: disabled)
#define PIN_STATUS_LED -1

#endif // CONFIG_H
