#include "AccModule.h"
#include "LoggerModule.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

static Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

int16_t Acc_Array[3] = {0};
AccStatus_t Acc_Status = ACC_STATUS_OK;

uint8_t Acc_Init(void)
{
    if (!accel.begin()) {
        LOG_ERROR("ADXL345 init fail");
        Acc_Status = ACC_STATUS_INIT_ERROR;
        return ACC_ERR_INIT;
    }
    // Set data rate ~ 400 Hz if possible
    // library may not let you pick exactly 400, but you can approximate
    accel.setRange(ADXL345_RANGE_16_G);
    LOG_DEBUG("accel.setRange complete.");

    accel.setDataRate(ADXL345_DATARATE_3200_HZ);
    LOG_DEBUG("accel.setDataRate complete.");

    Acc_Status = ACC_STATUS_OK;
    LOG_INFO("Acceleration module init OK");
    return ACC_ERR_OK;
}

uint8_t Acc_Read(void)
{
    if (Acc_Status == ACC_STATUS_INIT_ERROR) {
        return ACC_ERR_INIT;
    }
    sensors_event_t event;
    accel.getEvent(&event);

    // Convert float (m/s^2) to raw short. 
    // E.g. multiply by 10 to keep some resolution
    Acc_Array[0] = (int16_t)(event.acceleration.x * 10);
    Acc_Array[1] = (int16_t)(event.acceleration.y * 10);
    Acc_Array[2] = (int16_t)(event.acceleration.z * 10);

    LOG_DEBUG("Acc test: x=%d, y=%d, z=%d", Acc_Array[0], Acc_Array[1], Acc_Array[2]);

    Acc_Status = ACC_STATUS_OK;
    return ACC_ERR_OK;
}

void Acc_Test(void)
{
    uint8_t ret = Acc_Init();
    if (ret != ACC_ERR_OK) {
        LOG_ERROR("Acc test init fail: %d", ret);
        return;
    }
    ret = Acc_Read();
    if (ret != ACC_ERR_OK) {
        LOG_ERROR("Acc test read fail: %d", ret);
        return;
    }
    LOG_DEBUG("Acc test: x=%d, y=%d, z=%d", Acc_Array[0], Acc_Array[1], Acc_Array[2]);
    LOG_INFO("Acceleration test done.");
}
