#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>


// Preprocessor Definitions
#define BLE_MTU_Size 36

// https://www.uuidgenerator.net/

#define UUID_ACCEL_SERVICE_RIGHT "362791d5-c8be-477c-b20b-20eb05b1f008"
#define UUID_ACCEL_CHARACTERISTIC_RIGHT "4ef0a469-20f1-4b66-b20f-546df2d290bb"
#define UUID_PRESSURE_SERVICE_RIGHT "7549bdbe-03e8-46f2-9e6d-b1c2322d96b1"
#define UUID_PRESSURE_CHARACTERISTIC_RIGHT "394a95de-e2c0-4c65-9ec9-a4e72c958f58"
#define UUID_ACCEL_SERVICE_LEFT "da4df24d-a7d2-4470-98de-72e98321fed7"
#define UUID_ACCEL_CHARACTERISTIC_LEFT "71c0616f-b030-4d52-90f3-906be342fa0d"
#define UUID_PRESSURE_SERVICE_LEFT "06bca517-7044-4eca-b486-e2b9b1449787"
#define UUID_PRESSURE_CHARACTERISTIC_LEFT "10979ba1-aec0-4ac3-9a6b-1cc833808d4a"
#define Logger_BLE_Enabled 0
#define Logger_Serial_Enabled 1
#define PrintText_SerialFail -1
#define PrintText_BleFail -2
#define PrintText_BleAndSerialFail -3
#define PrintText_NA 0
#define PrintText_Success 1
#define LoggerDebugLevel 3
#define LoggerDebugLevelDefault 1
#define LoggerDebugLevel_Error 4
#define LoggerDebugLevel_Warning 3
#define LoggerDebugLevel_Info 2
#define LoggerDebugLevel_Debug 1
#define LoggerDebugLevel_NA 0
#define SideFlag 1 // 1 for right 0 for left


// Functions
void SetupLogger(void);
void SetupBL(void);
void SetupSerial(void);
void sendPressureArray(uint16_t *ArrayValues, size_t lengthValues, uint32_t timestamp);
void sendAccelerometerArray(uint16_t *ArrayValues, size_t lengthValues, uint32_t timestamp);
int PrintText(int DebugLevel, String OutputText);

void testLogger(void);
#endif
