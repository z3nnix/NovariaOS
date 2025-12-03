#ifndef LOG_H
#define LOG_H

#include <core/drivers/serial.h>

#define LOG_LEVEL_FATAL   0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4
#define LOG_LEVEL_TRACE   5

#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

static inline void log_message(const char* level, const char* msg) {
    serial_print("[");
    serial_print(level);
    serial_print("] ");
    serial_print(msg);
}

#define LOG_FATAL(msg) if (LOG_LEVEL_FATAL <= CURRENT_LOG_LEVEL) log_message("FATAL", msg)
#define LOG_ERROR(msg) if (LOG_LEVEL_ERROR <= CURRENT_LOG_LEVEL) log_message("ERROR", msg)
#define LOG_WARN(msg)  if (LOG_LEVEL_WARN <= CURRENT_LOG_LEVEL)  log_message("WARN", msg)
#define LOG_INFO(msg)  if (LOG_LEVEL_INFO <= CURRENT_LOG_LEVEL)  log_message("INFO", msg)
#define LOG_DEBUG(msg) if (LOG_LEVEL_DEBUG <= CURRENT_LOG_LEVEL) log_message("DEBUG", msg)
#define LOG_TRACE(msg) if (LOG_LEVEL_TRACE <= CURRENT_LOG_LEVEL) log_message("TRACE", msg)

#endif // LOG_H