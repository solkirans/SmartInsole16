#ifndef CONFIG_H
#define CONFIG_H

// I2C pins (ESP32)
#define I2C_SCL_Pin     22
#define I2C_SDA_Pin     21


// /////////////////////////////////////////////////////////////////
// ''''''' LOGGER ''''''''''''''''''' //

#define LOGGER_QUEUE_SIZE  10   // number of messages that can be queued
#define LOGGER_MAX_LOG_LENGTH  128  // max length per log message

// Logger task stack size
#define LOGGER_TASK_STACK_SIZE   16384

// Logging Levels
#define LOG_LEVEL_ERROR  0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO   2
#define LOG_LEVEL_DEBUG  3

// Selected_Logging Level
#define LOG_LEVEL_SELECTED  LOG_LEVEL_ERROR
// Serial output via logger enabled
#define LOG_SERIAL_ENABLED  0





// Default loop intervals and watchdog
#define DEFAULT_LOOP_INTERVAL_MS   20      // sensor loop: 100 Hz
#define DEBUG_LOOP_INTERVAL_MS   500      // sensor loop: 100 Hz
#define PRINT_INTERVAL      1000     // Print every 1000 ms if serial is enabled


#ifdef LOG_LEVEL_SELECTED
    #if LOG_LEVEL_SELECTED == LOG_LEVEL_DEBUG
        #define LOOP_INTERVAL_MS   DEBUG_LOOP_INTERVAL_MS     // Debug mode: 2 Hz
    #else
      #define LOOP_INTERVAL_MS   DEFAULT_LOOP_INTERVAL_MS          // Default: 50 Hz if LOG_LEVEL not defined
    #endif
#else
    #define LOOP_INTERVAL_MS   DEFAULT_LOOP_INTERVAL_MS          // Default: 50 Hz if LOG_LEVEL not defined
#endif

#define WATCHDOG_PERIOD_MS         3000    // Watchdog period

// /////////////////////////////////////////////////////////////////
// ''''''' BLE ''''''''''''''''''' //

// BLE delay after message sent
#define BLE_MSG_DELAY 1 // 1 ms

// BLE message length
#define BLE_MSG_LENGTH   39  // 1 byte battery, 6 bytes accel, 32 bytes pressure => 39 total


// BLE task stack size
#define BLE_TASK_STACK_SIZE   16384
// ------------------------------
// Advertising intervals
// ------------------------------
#define ADVERTISING_INTERVAL_MIN 0x50  // 0x50 * 0.625ms = ~31.25ms
#define ADVERTISING_INTERVAL_MAX 0x100 // 0x100 * 0.625ms = ~160ms

// /////////////////////////////////////////////////////////////////
// ''''''' SENSORS ''''''''''''''''''' //

// BLE task stack size
#define SENSOR_TASK_STACK_SIZE   16384


#endif // CONFIG_H
