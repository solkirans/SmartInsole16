#ifndef LOGGER_MODULE_H
#define LOGGER_MODULE_H

#include <Arduino.h>
#include "Config.h"

// Logging macros (function name & line number included)
#define LOG_ERROR(fmt, ...) LoggerPrint(LOG_LEVEL_ERROR, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) LoggerPrint(LOG_LEVEL_WARN, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt,  ...) LoggerPrint(LOG_LEVEL_INFO,  __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LoggerPrint(LOG_LEVEL_DEBUG, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

// Initialize the logger with desired level and whether to enable serial
void LoggerInit();

// Main logging function
void LoggerPrint(uint8_t level, const char* func, int line, const char* format, ...);

// FreeRTOS logger task
void LoggerTask(void* pvParams);

#endif // LOGGER_MODULE_H
