#ifndef CONFIG_H
#define CONFIG_H

// I2C pins (ESP32)
#define I2C_SCL_Pin     22
#define I2C_SDA_Pin     21

// Default loop intervals and watchdog
#define DEFAULT_LOOP_INTERVAL_MS   10      // sensor loop: 100 Hz
#define WATCHDOG_PERIOD_MS         250     // 250 ms
#define PRINT_INTERVAL_COUNT       100     // Print every 100 loops if serial is enabled

// Logging Levels
#define LOG_LEVEL_ERROR  0
#define LOG_LEVEL_INFO   1
#define LOG_LEVEL_DEBUG  2

// BLE message length
#define BLE_MSG_LENGTH   39  // 1 byte battery, 6 bytes accel, 32 bytes pressure => 39 total

// BLE queue shape
#define BLE_QUEUE_ROWS   100
#define BLE_QUEUE_COLS   BLE_MSG_LENGTH

#endif // CONFIG_H
