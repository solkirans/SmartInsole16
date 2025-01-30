#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>  // For fixed-width integer types (e.g., uint8_t, int16_t)
extern "C" {
  #include "esp_task_wdt.h"
}


// Common error definitions (shared across modules)
#define ERR_OK                     0x00
#define ERR_I2C_FAIL               0x01
#define ERR_SENSOR_INIT_FAIL       0x02
#define ERR_SENSOR_READ_FAIL       0x03
#define ERR_QUEUE_OVERFLOW         0x04
#define ERR_BATTERY_OVERVOLT       0x05
#define ERR_BATTERY_UNDERVOLT      0x06




#pragma pack(push, 1)  // Disable struct padding
typedef struct {
    uint32_t timestamp;   // 4 bytes - milliseconds since boot
    uint8_t battery;      // 1 byte
    int16_t accel_x;      // 2 bytes
    int16_t accel_y;      // 2 bytes
    int16_t accel_z;      // 2 bytes
    uint16_t pressure[16]; // 16 × 2 bytes = 32 bytes
} SensorData;             // Total size: 4 + 1 + 2 + 2 + 2 + 32 = 43 bytes
#pragma pack(pop)        // Restore default struct padding

#endif // COMMON_TYPES_H
