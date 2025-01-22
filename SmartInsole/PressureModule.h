#ifndef PRESSURE_MODULE_H
#define PRESSURE_MODULE_H

#include <Arduino.h>
#include "ErrorCodes.h"

#define PRESSURE_ERR_OK          ERR_OK
#define PRESSURE_ERR_INIT        ERR_SENSOR_INIT_FAIL
#define PRESSURE_ERR_READ        ERR_SENSOR_READ_FAIL

typedef enum {
    PRESSURE_STATUS_OK = 0,
    PRESSURE_STATUS_INIT_ERROR,
    PRESSURE_STATUS_READ_ERROR
} PressureStatus_t;

extern uint16_t Pressure_Array[16];
extern PressureStatus_t Pressure_Status;

// init
uint8_t Pressure_Init(void);
// read
uint8_t Pressure_Read(void);
// test
void Pressure_Test(void);

void Pressure_PrintValues(void);
#endif // PRESSURE_MODULE_H
