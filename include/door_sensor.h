#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <Arduino.h>
#include "config.h"

class DoorSensor {
public:
    DoorSensor(const DoorSensorConfig& config);

    void begin();
    // Call in main loop. Returns true if door state changed in this tick.
    bool update();

    bool isOpen() const;
    const char* getName() const;
    uint8_t getPin() const;

private:
    DoorSensorConfig m_config;
    bool m_isOpen;
    bool m_lastRawState;
    uint32_t m_lastDebounceTime;
};

#endif // DOOR_SENSOR_H
