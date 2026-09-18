#pragma once

#include <Arduino.h>
#include <esp_log.h>
#include <stdio.h>

namespace loggable {

enum class LogLevel {
    Verbose,
    Debug,
    Info,
    Warning,
    Error
};

class Loggable {
public:
    explicit Loggable(const char* tag = "PN532") : _tag(tag) {}
    virtual ~Loggable() = default;
protected:
    const char* _tag;
};

} // namespace loggable

#ifndef LOG
#define LOG(level, msg, ...) do { \
    if ((level) == loggable::LogLevel::Error) { \
        ESP_LOGE("PN532", msg, ##__VA_ARGS__); \
    } else if ((level) == loggable::LogLevel::Warning) { \
        ESP_LOGW("PN532", msg, ##__VA_ARGS__); \
    } else if ((level) == loggable::LogLevel::Info) { \
        ESP_LOGI("PN532", msg, ##__VA_ARGS__); \
    } else { \
        ESP_LOGD("PN532", msg, ##__VA_ARGS__); \
    } \
} while(0)
#endif
