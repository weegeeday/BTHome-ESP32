#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H

#include <Arduino.h>
#include "config.h"

class LightControl {
public:
    LightControl(const LightConfig& config);

    void begin();
    // Update button sensing if physical button pin is configured. Returns true if state toggled.
    bool update();

    void setLight(bool turnOn);
    void toggleLight();
    bool isOn() const;
    const char* getName() const;
    uint8_t getRelayPin() const;

private:
    LightConfig m_config;
    bool m_isOn;
    bool m_lastButtonState;
    uint32_t m_lastButtonDebounce;
    bool m_buttonPressedState;
};

#endif // LIGHT_CONTROL_H
