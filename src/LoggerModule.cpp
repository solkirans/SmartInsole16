#include "LoggerModule.h"
#include <stdarg.h>

static const size_t LOG_QUEUE_SIZE  = LOGGER_QUEUE_SIZE;   // number of messages that can be queued
static const size_t MAX_LOG_LENGTH  = LOGGER_MAX_LOG_LENGTH;  // max length per log message

static uint8_t s_logLevel      = LOGGER_LEVEL_ERROR;
static bool    s_serialEnabled = false;
static unsigned long lastPrintTime = 0;
typedef struct {
    char msg[MAX_LOG_LENGTH];
    uint8_t level;
} LogItem_t;

static QueueHandle_t s_loggerQueue = nullptr;

void LoggerInit()
{
    s_logLevel = LOG_LEVEL_SELECTED;
    s_serialEnabled = LOG_SERIAL_ENABLED;

    Serial.begin(115200);
    // Create the queue if not created
    if (!s_loggerQueue) {
        s_loggerQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogItem_t));
    }
}

void LoggerPrint(uint8_t level, const char* func, int line, const char* format, ...)
{
    // If level is above current log level, skip
    if ((level > s_logLevel) || (!s_serialEnabled)) {
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

// Prints the sensor byte array if enough time has passed
void LoggerPrintLoopMessage(SensorData* sensor_msg) {
    
    unsigned long currentTime = millis();
    
    if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = currentTime;

        // Create debug string with all values in decimal
        String dbg;
        
        // Battery
        dbg += "Batt: ";
        dbg += String(sensor_msg->battery);
        dbg += " | ";

        // Accelerometer
        dbg += "Accel(";
        dbg += String(sensor_msg->accel_x);
        dbg += ", ";
        dbg += String(sensor_msg->accel_y);
        dbg += ", ";
        dbg += String(sensor_msg->accel_z);
        dbg += ") | ";

        // Pressure array
        dbg += "Press[";
        for (int i = 0; i < 16; i++) {
            dbg += String(sensor_msg->pressure[i]);
            if (i < 15) dbg += ", ";
        }
        dbg += "]";

        LOG_INFO("TxMsg: %s", dbg.c_str());
    }
}