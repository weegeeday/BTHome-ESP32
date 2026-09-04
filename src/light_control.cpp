#include "light_control.h"

LightControl::LightControl(const LightConfig& config)
    : m_config(config),
      m_isOn(false),
      m_lastButtonState(HIGH),
      m_lastButtonDebounce(0),
      m_buttonPressedState(false) {}

void LightControl::begin() {
    pinMode(m_config.relayPin, OUTPUT);
    setLight(false); // Default to off

    if (m_config.buttonPin >= 0) {
        if (m_config.buttonPullup) {
            pinMode(m_config.buttonPin, INPUT_PULLUP);
        } else {
            pinMode(m_config.buttonPin, INPUT);
        }
        m_lastButtonState = digitalRead(m_config.buttonPin);
    }
}

void LightControl::setLight(bool turnOn) {
    m_isOn = turnOn;
    digitalWrite(m_config.relayPin, (m_isOn == m_config.activeHigh) ? HIGH : LOW);
}

void LightControl::toggleLight() {
    setLight(!m_isOn);
}

bool LightControl::isOn() const {
    return m_isOn;
}

const char* LightControl::getName() const {
    return m_config.name;
}

uint8_t LightControl::getRelayPin() const {
    return m_config.relayPin;
}

bool LightControl::update() {
    if (m_config.buttonPin < 0) {
        return false;
    }

    bool currentReading = digitalRead(m_config.buttonPin);
    if (currentReading != m_lastButtonState) {
        m_lastButtonDebounce = millis();
        m_lastButtonState = currentReading;
    }

    if ((millis() - m_lastButtonDebounce) > 50) {
        bool isPressed = m_config.buttonPullup ? (currentReading == LOW) : (currentReading == HIGH);
        
        if (isPressed && !m_buttonPressedState) {
            m_buttonPressedState = true;
            toggleLight();
            return true; // Light state changed via physical button
        } else if (!isPressed && m_buttonPressedState) {
            m_buttonPressedState = false;
        }
    }

    return false;
}
