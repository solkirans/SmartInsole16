#ifndef ACCELEROMETER_MODULE_H
#define ACCELEROMETER_MODULE_H
#include <Wire.h>


#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>

// Preprocessor Definitions
#define ADXL345_Range_2g 0x00
#define ADXL345_Range_4g 0x01
#define ADXL345_Range_8g 0x02
#define ADXL345_Range_16g 0x03
#define ADXL345_Range ADXL345_Range_4g

#define ADXL345_DataRate_25Hz 0x08
#define ADXL345_DataRate_50Hz 0x09
#define ADXL345_DataRate_100Hz 0x0A
#define ADXL345_DataRate_200Hz 0x0B
#define ADXL345_DataRate_400Hz 0x0C
#define ADXL345_DataRate_800Hz 0x0D
#define ADXL345_DataRate_1600Hz 0x0E
#define ADXL345_DataRate_3200Hz 0x0F

#define ADXL345_POWER_CTL_Standby 0x00
#define ADXL345_POWER_CTL_Measurement 0x08

#define ADXL345_FIFO_CTL_Bypass 0x00
#define ADXL345_FIFO_CTL_FIFO 0x40
#define ADXL345_FIFO_CTL_Stream 0x80
#define ADXL345_FIFO_CTL_Trigger 0xC0


#define ADXL345_DataLength 3

// Function Prototypes
bool AccelerometerSetup(void);
bool AccelerometerRead(void);
void TestAccelerometerModule(void);
extern uint16_t AccelerometerValues[ADXL345_DataLength];
extern uint32_t AccelerometerTimestamp;
#endif // ACCELEROMETER_MODULE_H
