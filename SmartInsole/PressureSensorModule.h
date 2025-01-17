#ifndef PRESSURE_SENSOR_MODULE_H
#define PRESSURE_SENSOR_MODULE_H

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <string.h>
// Preprocessor Definitions
#define ADS1115_Status_Ok 0
#define ADS1115_Status_UnknownError 1
#define ADS1115_Status_InitFail 2
#define ADS1115_Status_ReadBeginFail 3
#define ADS1115_Status_ReadFail 4

#define ADS1115_Gain GAIN_ONE // Gain_ONE (4.096V range)
#define ADS1115_SPS_250 0x00A0
#define ADS1115_SPS ADS1115_SPS_250

#define I2C_SCL_Pin 22
#define I2C_SDA_Pin 21
#define ADS1115_Count 4
#define ADS1115_Channel_Count 4
#define ADS1115_TotalChannel_Count (ADS1115_Count * ADS1115_Channel_Count)

// Variables (declared as extern to prevent multiple definitions)
extern const uint8_t ADS1115_Address[ADS1115_Count];
extern uint16_t PressureSensorValues[ADS1115_TotalChannel_Count];
extern uint32_t PressureMeasurementTimeStamp;
extern uint8_t ADS1115_Status[ADS1115_Count];

// Function Prototypes
bool ADCSetup();
bool ADCRead();
void testPressureSensorModule();

#endif // PRESSURE_SENSOR_MODULE_H
