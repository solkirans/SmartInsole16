#include "LoggerModule.h"
#include <stdarg.h>

static const size_t LOG_QUEUE_SIZE  = 20;   // number of messages that can be queued
static const size_t MAX_LOG_LENGTH  = 128;  // max length per log message

static uint8_t s_logLevel      = LOG_LEVEL_ERROR;
static bool    s_serialEnabled = false;

typedef struct {
    char msg[MAX_LOG_LENGTH];
    uint8_t level;
} LogItem_t;

static QueueHandle_t s_loggerQueue = nullptr;

void LoggerInit(uint8_t logLevel, bool serialEnable)
{
    s_logLevel = logLevel;
    s_serialEnabled = serialEnable;

    if (s_serialEnabled) {
        Serial.begin(115200);
    }
    // Create the queue if not created
    if (!s_loggerQueue) {
        s_loggerQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogItem_t));
    }
}

void LoggerPrint(uint8_t level, const char* func, int line, const char* format, ...)
{
    // If level is above current log level, skip
    if (level > s_logLevel) {
        return;
    }

    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    // Prepend function & line
    char finalMsg[MAX_LOG_LENGTH];
    snprintf(finalMsg, sizeof(finalMsg), "[%s:%d] %s", func, line, buf);

    // Enqueue
    if (s_loggerQueue) {
        LogItem_t item;
        strncpy(item.msg, finalMsg, MAX_LOG_LENGTH);
        item.msg[MAX_LOG_LENGTH-1] = '\0';
        item.level = level;
        xQueueSend(s_loggerQueue, &item, 0);
    }
}

void LoggerTask(void* pvParams)
{
    LogItem_t item;
    for (;;) {
        // Wait for next log message
        if (xQueueReceive(s_loggerQueue, &item, portMAX_DELAY) == pdTRUE) {
            if (s_serialEnabled) {
                Serial.println(item.msg);
            }
            // If serial not enabled, we discard or could store logs in memory
        }
    }
}
