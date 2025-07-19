#include "../include/log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static log_level_t current_log_level = LOG_LEVEL_INFO;

void set_log_level(log_level_t level) {
    current_log_level = level;
}

static void log_message(log_level_t level, const char *level_str, const char *format, va_list args) {
    if (level > current_log_level) {
        return;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] %s: ", time_str, level_str);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_ERROR, "ERROR", format, args);
    va_end(args);
}

void log_warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_WARN, "WARN", format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_INFO, "INFO", format, args);
    va_end(args);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_DEBUG, "DEBUG", format, args);
    va_end(args);
} 