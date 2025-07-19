#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

// Log levels
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} log_level_t;

// Log functions
void log_error(const char *format, ...);
void log_warn(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);

// Set log level
void set_log_level(log_level_t level);

// Log macros for easier use
#define LOG_ERROR(fmt, ...) log_error("[ERROR] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_warn("[WARN]  " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_info("[INFO]  " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log_debug("[DEBUG] " fmt, ##__VA_ARGS__)

#endif // LOG_H 