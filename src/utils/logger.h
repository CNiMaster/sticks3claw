#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

class Logger {
public:
    // 日志级别
    enum Level {
        LEVEL_NONE = 0,
        LEVEL_ERROR = 1,
        LEVEL_WARN = 2,
        LEVEL_INFO = 3,
        LEVEL_DEBUG = 4,
        LEVEL_VERBOSE = 5
    };

    // 设置日志级别
    static void setLevel(Level level);

    // 日志方法
    static void error(const char* tag, const char* format, ...);
    static void warn(const char* tag, const char* format, ...);
    static void info(const char* tag, const char* format, ...);
    static void debug(const char* tag, const char* format, ...);
    static void verbose(const char* tag, const char* format, ...);

    // 便捷宏
    #define LOGE(tag, ...) Logger::error(tag, __VA_ARGS__)
    #define LOGW(tag, ...) Logger::warn(tag, __VA_ARGS__)
    #define LOGI(tag, ...) Logger::info(tag, __VA_ARGS__)
    #define LOGD(tag, ...) Logger::debug(tag, __VA_ARGS__)
    #define LOGV(tag, ...) Logger::verbose(tag, __VA_ARGS__)

private:
    static Level _level;
    static void log(Level level, const char* tag, const char* format, va_list args);
};

#endif
