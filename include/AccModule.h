#ifndef ACC_MODULE_H
#define ACC_MODULE_H

#include <Arduino.h>
#include "CommonTypes.h"

// /////////////////////////////////////////////////////////////////
// ''''''' ACCELERATION SENSOR  ''''''''''''''''''' //
#define ACC_ERR_OK        ERR_OK
#define ACC_ERR_INIT      ERR_SENSOR_INIT_FAIL
#define ACC_ERR_READ      ERR_SENSOR_READ_FAIL

typedef enum {
    ACC_STATUS_OK = 0,
    ACC_STATUS_INIT_ERROR,
    ACC_STATUS_READ_ERROR
} AccStatus_t;

extern int16_t Acc_Array[3];
extern AccStatus_t Acc_Status;

uint8_t Acc_Init(void);
uint8_t Acc_Read(void);
void Acc_Test(void);

#endif
