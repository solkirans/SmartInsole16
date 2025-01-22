#include "PressureModule.h"
#include "LoggerModule.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// Example addresses: 0x48, 0x49, 0x4A, 0x4B
static const uint8_t ADS1115_ADDR[4] = {0x48, 0x49, 0x4A, 0x4B};

static Adafruit_ADS1115 ads[4];
uint16_t Pressure_Array[16] = {0};
PressureStatus_t Pressure_Status = PRESSURE_STATUS_OK;

uint8_t Pressure_Init(void)
{
    for (int i = 0; i < 4; i++) {
        ads[i] = Adafruit_ADS1115(); // use default constructor
        LOG_DEBUG("Adafruit_ADS1115 object creation complete.");

        if (!ads[i].begin(ADS1115_ADDR[i])) {
            LOG_ERROR("ADS1115 init failed at addr 0x%02X", ADS1115_ADDR[i]);
            Pressure_Status = PRESSURE_STATUS_INIT_ERROR;
            return PRESSURE_ERR_INIT;
        }
        LOG_DEBUG("ads[i].begin(ADS1115_ADDR[i]) complete.");
        // Configure for single-shot, 860SPS, gain=1, etc.
        ads[i].setGain(GAIN_ONE);
        LOG_DEBUG("ads[i].setGain complete.");

        ads[i].setDataRate(RATE_ADS1115_128SPS);
        LOG_DEBUG("ads[i].setDataRate complete.");
        delay(100);
    }
    Pressure_Status = PRESSURE_STATUS_OK;
    LOG_INFO("Pressure module init OK.");
    return PRESSURE_ERR_OK;
}

uint8_t Pressure_Read(void)
{
    if (Pressure_Status == PRESSURE_STATUS_INIT_ERROR) {
        return PRESSURE_ERR_INIT;
    }

    // Read 4 channels from each ADS1115
    for (int dev = 0; dev < 4; dev++) {
        for (int ch = 0; ch < 4; ch++) {
            int16_t raw = ads[dev].readADC_SingleEnded(ch);
            if (raw < 0) {
                LOG_ERROR("ADS1115 read error: dev=%d ch=%d", dev, ch);
                Pressure_Status = PRESSURE_STATUS_READ_ERROR;
                return PRESSURE_ERR_READ;
            }
            Pressure_Array[dev * 4 + ch] = (uint16_t) raw;
        }
    }

    Pressure_PrintValues();


    Pressure_Status = PRESSURE_STATUS_OK;
    return PRESSURE_ERR_OK;
}

void Pressure_PrintValues(void)
{
      char debug_str[128];
    snprintf(debug_str, sizeof(debug_str), 
    "[0]=%u [1]=%u [2]=%u [3]=%u [4]=%u [5]=%u [6]=%u [7]=%u ",
    Pressure_Array[0], Pressure_Array[1], Pressure_Array[2], Pressure_Array[3],
    Pressure_Array[4], Pressure_Array[5], Pressure_Array[6], Pressure_Array[7]);
    LOG_DEBUG("%s", debug_str);

    snprintf(debug_str, sizeof(debug_str), 
    "[8]=%u [9]=%u [10]=%u [11]=%u [12]=%u [13]=%u [14]=%u [15]=%u",
    Pressure_Array[8], Pressure_Array[9], Pressure_Array[10], Pressure_Array[11],
    Pressure_Array[12], Pressure_Array[13], Pressure_Array[14], Pressure_Array[15]);
    LOG_DEBUG("%s", debug_str);
}

void Pressure_Test(void)
{
    uint8_t ret = Pressure_Init();
    if (ret != PRESSURE_ERR_OK) {
        LOG_ERROR("Pressure test init fail: %d", ret);
        return;
    }
    ret = Pressure_Read();
    if (ret != PRESSURE_ERR_OK) {
        LOG_ERROR("Pressure test read fail: %d", ret);
        return;
    }
    Pressure_PrintValues();
    LOG_INFO("Pressure test done.");
}
