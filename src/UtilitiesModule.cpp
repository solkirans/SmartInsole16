#include "UtilitiesModule.h"
#include <Wire.h>
#include <Adafruit_MAX1704X.h>

static Adafruit_MAX17048 maxlipo;
uint8_t BatteryVoltage = 0;
BatteryStatus_t Battery_Status = BATTERY_STATUS_OK;
static uint32_t s_lastReadTime = 0;


void deviceResetReason(){
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.print("Reset reason: ");
    switch (reason) {
        case ESP_RST_POWERON:   LOG_DEBUG("ESP_RST_POWERON"); break;
        case ESP_RST_EXT:       LOG_DEBUG("ESP_RST_EXT"); break;
        case ESP_RST_SW:        LOG_DEBUG("ESP_RST_SW"); break;
        case ESP_RST_PANIC:     LOG_DEBUG("ESP_RST_PANIC"); break;
        case ESP_RST_INT_WDT:   LOG_DEBUG("ESP_RST_INT_WDT"); break;
        case ESP_RST_TASK_WDT:  LOG_DEBUG("ESP_RST_TASK_WDT"); break;
        case ESP_RST_WDT:       LOG_DEBUG("ESP_RST_WDT"); break;  // Generic WDT
        case ESP_RST_DEEPSLEEP: LOG_DEBUG("ESP_RST_DEEPSLEEP"); break;
        case ESP_RST_BROWNOUT:  LOG_DEBUG("ESP_RST_BROWNOUT"); break;
        case ESP_RST_SDIO:      LOG_DEBUG("ESP_RST_SDIO"); break;
        default:                LOG_DEBUG("UNKNOWN"); break;
    }
}

// Example: We want ~30 seconds total WDT, using 3 stages of ~10s each
// Use prescaler so each tick = 1 ms => prescaler = 80 MHz / 80,000 = 1 kHz
// Then 10s => 10,000 ticks per stage

// Function to pack sensor data into BLE message
void PackSensorData(SensorData &sensor_data) {
  
    // Prepare the message and add it to the struct
    
    // Add battery data
    sensor_data.battery = BatteryVoltage;
    
    // Add accelerometer 
    sensor_data.accel_x = Acc_Array[0];
    sensor_data.accel_y = Acc_Array[1];
    sensor_data.accel_z = Acc_Array[2];
    
    // Copy the pressure data into the struct
    memcpy(sensor_data.pressure, Pressure_Array, sizeof(sensor_data.pressure));
}

// Clear all struct fields to zero
void clearSensorData(SensorData* data) {
    if (data != nullptr) {
        memset(data, 0, sizeof(SensorData));
    }
}


void addDummyData(SensorData &sensor_data){
    // Generate a random number between -1 and 1
    float randomValue = ((float)random(-1000, 1000) / 1000.0) * 2.0 - 1.0;

    // Add timestamp
    sensor_data.timestamp = millis();

    // Add random value to battery and accelerometer data
    sensor_data.battery = 100 + randomValue;
    sensor_data.accel_x = 123 + randomValue;
    sensor_data.accel_y = 456 + randomValue;
    sensor_data.accel_z = 789 + randomValue;

    // Multiply random value by 100 and add to pressure data
    for (int i = 0; i < 16; i++) 
    {
        sensor_data.pressure[i] = 1000 + i + (randomValue * 100);
    }
}


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