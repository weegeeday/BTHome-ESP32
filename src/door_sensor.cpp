#include "door_sensor.h"

DoorSensor::DoorSensor(const DoorSensorConfig& config)
    : m_config(config),
      m_isOpen(false),
      m_lastRawState(false),
      m_lastDebounceTime(0) {}

void DoorSensor::begin() {
    if (m_config.usePullup) {
        pinMode(m_config.pin, INPUT_PULLUP);
    } else {
        pinMode(m_config.pin, INPUT);
    }

    int rawRead = digitalRead(m_config.pin);
    m_isOpen = m_config.activeLow ? (rawRead == LOW) : (rawRead == HIGH);
    m_lastRawState = m_isOpen;
}

bool DoorSensor::update() {
    int rawRead = digitalRead(m_config.pin);
    bool currentRawState = m_config.activeLow ? (rawRead == LOW) : (rawRead == HIGH);

    if (currentRawState != m_lastRawState) {
        m_lastDebounceTime = millis();
        m_lastRawState = currentRawState;
    }

    if ((millis() - m_lastDebounceTime) > m_config.debounceMs) {
        if (currentRawState != m_isOpen) {
            m_isOpen = currentRawState;
            return true; // State changed!
        }
    }

    return false;
}

bool DoorSensor::isOpen() const {
    return m_isOpen;
}

const char* DoorSensor::getName() const {
    return m_config.name;
}

uint8_t DoorSensor::getPin() const {
    return m_config.pin;
}
