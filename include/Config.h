#ifndef CONFIG_H
#define CONFIG_H

// ******************************************
// Device Configuration
// ******************************************
#define DEVICE_SIDE_LEFT     0
#define DEVICE_SIDE_RIGHT    1
#define g_sideFlag           DEVICE_SIDE_LEFT  // Select device side
#define testDeviceBLE        1                 // Set to 1 for BLE testing with dummy data

// ******************************************
// Hardware Configuration
// ******************************************
// I2C Settings
#define I2C_SCL_Pin         22
#define I2C_SDA_Pin         21
#define I2C_FREQUENCY       400000  // 400kHz

// ******************************************
// Task Configuration
// ******************************************
// Task Priorities (higher number = higher priority)
#define TASK_PRIORITY_LOGGER     1
#define TASK_PRIORITY_SENSOR     3    // Increased priority
#define TASK_PRIORITY_BLE        4    // Highest priority for consistent timing

// Core Pinning (ESP32 has 2 cores)
#define CORE_LOGGER     0    // Logger on core 0
#define CORE_SENSOR     1    // Sensor on core 1
#define CORE_BLE        0    // BLE on core 0

// Task Stack Sizes
#define LOGGER_TASK_STACK_SIZE   4096
#define SENSOR_TASK_STACK_SIZE   4096
#define BLE_TASK_STACK_SIZE      8192

// ******************************************
// Timing Configuration (milliseconds)
// ******************************************
#define LOOP_INTERVAL_MS         20    // Sensor sampling at 50Hz
#define BLE_TX_INTERVAL_MS       20    // BLE transmission at 50Hz (matched with sampling)
#define PRINT_INTERVAL           5000  // Status print interval
#define WATCHDOG_PERIOD          5    // Watchdog timeout in seconds

// Timing Tolerances
#define BLE_TX_TIMING_TOLERANCE_MS     5  // Maximum allowed deviation from BLE interval
#define SENSOR_TIMING_TOLERANCE_MS     5  // Maximum allowed deviation from sensor interval

// ******************************************
// Logger Configuration
// ******************************************
#define LOG_LEVEL_SELECTED    CORE_DEBUG_LEVEL
#define LOGGER_QUEUE_SIZE     25      // Reduced from 50 to save memory
#define LOGGER_MAX_LOG_LENGTH 128     // Reduced from 256 to save memory

// Logging Levels
#define LOGGER_LEVEL_NA      0
#define LOGGER_LEVEL_ERROR   1
#define LOGGER_LEVEL_WARN    2
#define LOGGER_LEVEL_INFO    3
#define LOGGER_LEVEL_DEBUG   4
#define LOGGER_LEVEL_VERBOSE 5

// ******************************************
// BLE Configuration
// ******************************************
// Recovery Settings
#define BLE_MAX_ERRORS_BEFORE_RECOVERY  3
#define BLE_RECOVERY_ATTEMPTS           2
#define BLE_RECOVERY_DELAY_MS           500

// Mutex Timeout
#define MUTEX_TIMEOUT_MS               2     // Reduced from 5ms for tighter timing

// ******************************************
// Sensor Configuration
// ******************************************
// Battery Reading Interval
#define BATTERY_READ_INTERVAL_MS    (5 * 60 * 1000)  // 5 minutes

#endif // CONFIG_H
