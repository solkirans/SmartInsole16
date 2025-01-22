#include "UtilitiesModule.h"
#include "LoggerModule.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_MAX1704X.h>

static Adafruit_MAX17048 maxlipo;
uint8_t BatteryVoltage = 0;
BatteryStatus_t Battery_Status = BATTERY_STATUS_OK;
static uint32_t s_lastReadTime = 0;

uint8_t Battery_Init(void)
{
    if (!maxlipo.begin()) {
        LOG_ERROR("MAX17048 init fail");
        Battery_Status = BATTERY_STATUS_READ_ERROR;
        return BATT_ERR_INIT;
    }
    Battery_Status = BATTERY_STATUS_OK;
    LOG_INFO("Battery module init OK");
    return BATT_ERR_OK;
}

uint8_t Battery_Read(void)
{ 
    if (Battery_Status)
    {
      return Battery_Status;
    }
    uint32_t now = millis();
    if ((now - s_lastReadTime) < BATT_READ_PERIOD_MS && s_lastReadTime != 0) {
        // skip reading if not enough time has passed
        return BATT_ERR_OK;
    }
    s_lastReadTime = now;

    float voltage = maxlipo.cellVoltage();
    if (voltage < 0) {
        LOG_ERROR("Battery read fail");
        Battery_Status = BATTERY_STATUS_READ_ERROR;
        return BATT_ERR_READ;
    }

    // Over/Under check
    if (voltage >= 6.0f) {
        BatteryVoltage = 255; 
        Battery_Status = BATTERY_STATUS_OVERVOLT;
    } else if (voltage <= 0.0f) {
        BatteryVoltage = 5;  
        Battery_Status = BATTERY_STATUS_UNDERVOLT;
    } else {
        // scale 0..6 => 10..250
        float ratio = voltage / 6.0f; 
        BatteryVoltage = (uint8_t)(ratio * 240.0f + 10.0f);
        Battery_Status = BATTERY_STATUS_OK;
    }
    LOG_DEBUG("Battery test reading: %d", BatteryVoltage);
    return BATT_ERR_OK;
}

void Battery_Test(void)
{
    uint8_t ret = Battery_Init();
    if (ret != BATT_ERR_OK) {
        LOG_ERROR("Battery test init fail: %d", ret);
        return;
    }
    ret = Battery_Read();
    if (ret != BATT_ERR_OK) {
        LOG_ERROR("Battery test read fail: %d", ret);
        return;
    }
    LOG_DEBUG("Battery test reading: %d", BatteryVoltage);
    LOG_INFO("Battery test done.");
}

void i2cScanner()
{
    // Print initial scanner message:
    LOG_INFO("I2C Scanner: Scanning for devices...");

    // Scan each possible 7-bit I2C address
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            // Device at this address responded
            char tmpMsg[64];
            snprintf(tmpMsg, sizeof(tmpMsg), 
                     "I2C device found at address 0x%02X!", address);
            LOG_INFO("%s", tmpMsg);
        } 
        else if (error == 4) {
            // "Other error" in Wire lib
            char tmpMsg[64];
            snprintf(tmpMsg, sizeof(tmpMsg),
                     "Unknown error at address 0x%02X", address);
            LOG_INFO("%s", tmpMsg);
        }
    }

    // Print final message after the scan:
    LOG_INFO("I2C scan complete.");
}