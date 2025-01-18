#ifndef UTILITIES_MODULE_H
#define UTILITIES_MODULE_H

#include <Arduino.h>
#include "ErrorCodes.h"

#define BATT_ERR_OK         ERR_OK
#define BATT_ERR_INIT       ERR_SENSOR_INIT_FAIL
#define BATT_ERR_READ       ERR_SENSOR_READ_FAIL

#define BATT_READ_PERIOD_MS (5 * 60 * 1000) // 5 minutes default

typedef enum {
    BATTERY_STATUS_OK = 0,
    BATTERY_STATUS_OVERVOLT,
    BATTERY_STATUS_UNDERVOLT,
    BATTERY_STATUS_READ_ERROR
} BatteryStatus_t;

extern uint8_t BatteryVoltage;       // 0..255 scaled
extern BatteryStatus_t Battery_Status;

uint8_t Battery_Init(void);
uint8_t Battery_Read(void);
void Battery_Test(void);

#endif
