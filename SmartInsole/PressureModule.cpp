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
        if (!ads[i].begin(ADS1115_ADDR[i])) {
            LOG_ERROR("ADS1115 init failed at addr 0x%02X", ADS1115_ADDR[i]);
            Pressure_Status = PRESSURE_STATUS_INIT_ERROR;
            return PRESSURE_ERR_INIT;
        }
        // Configure for single-shot, 860SPS, gain=1, etc.
        ads[i].setGain(GAIN_ONE);
        ads[i].setDataRate(RATE_ADS1115_860SPS);
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
    Pressure_Status = PRESSURE_STATUS_OK;
    return PRESSURE_ERR_OK;
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
    for (int i = 0; i < 16; i++) {
        LOG_DEBUG("Pressure[%d]=%u", i, Pressure_Array[i]);
    }
    LOG_INFO("Pressure test done.");
}
