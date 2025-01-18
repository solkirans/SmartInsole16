#ifndef ERROR_CODES_H
#define ERROR_CODES_H

// Common error definitions (shared across modules)
#define ERR_OK                     0x00
#define ERR_I2C_FAIL               0x01
#define ERR_SENSOR_INIT_FAIL       0x02
#define ERR_SENSOR_READ_FAIL       0x03
#define ERR_QUEUE_OVERFLOW         0x04
#define ERR_BATTERY_OVERVOLT       0x05
#define ERR_BATTERY_UNDERVOLT      0x06

#endif // ERROR_CODES_H
