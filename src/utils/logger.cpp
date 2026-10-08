#include "logger.h"
#include <stdarg.h>

Logger::Level Logger::_level = Logger::LEVEL_INFO;

void Logger::setLevel(Level level) {
    _level = level;
}

void Logger::error(const char* tag, const char* format, ...) {
    if (_level >= LEVEL_ERROR) {
        va_list args;
        va_start(args, format);
        log(LEVEL_ERROR, tag, format, args);
        va_end(args);
    }
}

void Logger::warn(const char* tag, const char* format, ...) {
    if (_level >= LEVEL_WARN) {
        va_list args;
        va_start(args, format);
        log(LEVEL_WARN, tag, format, args);
        va_end(args);
    }
}

void Logger::info(const char* tag, const char* format, ...) {
    if (_level >= LEVEL_INFO) {
        va_list args;
        va_start(args, format);
        log(LEVEL_INFO, tag, format, args);
        va_end(args);
    }
}

void Logger::debug(const char* tag, const char* format, ...) {
    if (_level >= LEVEL_DEBUG) {
        va_list args;
        va_start(args, format);
        log(LEVEL_DEBUG, tag, format, args);
        va_end(args);
    }
}

void Logger::verbose(const char* tag, const char* format, ...) {
    if (_level >= LEVEL_VERBOSE) {
        va_list args;
        va_start(args, format);
        log(LEVEL_VERBOSE, tag, format, args);
        va_end(args);
    }
}

void Logger::log(Level level, const char* tag, const char* format, va_list args) {
    // 打印时间戳
    unsigned long now = millis();
    unsigned long seconds = now / 1000;
    unsigned long ms = now % 1000;

    Serial.printf("[%05lu.%03lu] ", seconds, ms);

    // 打印级别
    switch (level) {
        case LEVEL_ERROR: Serial.print("[E] "); break;
        case LEVEL_WARN:  Serial.print("[W] "); break;
        case LEVEL_INFO:  Serial.print("[I] "); break;
        case LEVEL_DEBUG: Serial.print("[D] "); break;
        case LEVEL_VERBOSE: Serial.print("[V] "); break;
        default: Serial.print("[?] "); break;
    }

    // 打印标签
    Serial.printf("[%s] ", tag);

    // 打印格式化消息
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.println(buffer);
}
