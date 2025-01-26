#ifndef UTILITIES_MODULE_H
#define UTILITIES_MODULE_H

#include <Arduino.h>
#include "CommonTypes.h"
#include "LoggerModule.h"
#include "PressureModule.h"
#include "AccModule.h"
#include "LoggerModule.h"
#include "Config.h"
extern "C" {
  #include "esp_task_wdt.h"
}

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
void i2cScanner(void);
void PackSensorData(SensorData &sensor_data);
void setupTimerGroupWDT();
void feedHardwareWDT();
void clearSensorData(SensorData* data);
void addDummyData(SensorData &sensor_data);
void deviceResetReason();

#endif
