#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "LoggerModule.h"
#include "PressureModule.h"
#include "AccModule.h"
#include "UtilitiesModule.h"
#include "BluetoothModule.h"
#include "ErrorCodes.h"

// Globals
static bool g_sideFlag = 1;  // 0 => left, 1 => right
static unsigned long s_lastTaskTime = 0; // for watchdog
// 1) Sensor Task
void SensorTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DEFAULT_LOOP_INTERVAL_MS);

    for(;;) {
        //vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Watchdog check
        unsigned long now = millis();
        if ((now - s_lastTaskTime) > WATCHDOG_PERIOD_MS && s_lastTaskTime != 0) {
            LOG_ERROR("Watchdog triggered, system restarting...");
            ESP.restart();
        }
        s_lastTaskTime = now;
          // Read sensors
        
        Battery_Read();
        Acc_Read();
        Pressure_Read();

    }
}

// 2) Communication Task
void CommunicationTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DEFAULT_LOOP_INTERVAL_MS);

    static uint32_t loopCount = 0;

    for(;;) {

        //vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // Build 39-byte payload:
        //   [0]   = BatteryVoltage (1 byte)
        //   [1..6] = Acc_Array (3× int16 => 6 bytes)
        //   [7..38] = Pressure_Array (16× uint16 => 32 bytes)
        uint8_t msg[BLE_MSG_LENGTH];
        int idx = 0;

        // Battery
        msg[idx++] = BatteryVoltage;

        // Acc (3×int16 => 6 bytes)
        for(int i=0; i<3; i++) {
            int16_t val = Acc_Array[i];
            msg[idx++] = (uint8_t)( val       & 0xFF);
            msg[idx++] = (uint8_t)((val >> 8) & 0xFF);
        }

        // Pressure (16×uint16 => 32 bytes)
        for(int i=0; i<16; i++){
            uint16_t pv = Pressure_Array[i];
            msg[idx++] = (uint8_t)( pv       & 0xFF);
            msg[idx++] = (uint8_t)((pv >> 8) & 0xFF);
        }

        // Send via BLE
        BLE_SendByteArray(msg);
        vTaskDelay(1); // give BLE stack a moment to send

        // Optionally print info if serial is enabled
        loopCount++;
        if (loopCount >= PRINT_INTERVAL_COUNT) {
            loopCount = 0;
            // If you want to print the array in ASCII or hex:
            String dbg;
            for (int i=0; i<BLE_MSG_LENGTH; i++) {
                dbg += String(msg[i], HEX) + " ";
            }
            LOG_INFO("TxMsg: %s", dbg.c_str());
        }

    }
}

// 3) The usual Arduino setup
void setup()
{
    // 1. Logger init: let’s say we want INFO logs, with serial enabled
    LoggerInit(LOG_LEVEL_ERROR, true);

    // 2. Create logger task
    xTaskCreate(LoggerTask, "LoggerTask", 4096, NULL, 1, NULL);

    // 3. Initialize I2C
    Wire.begin(I2C_SDA_Pin, I2C_SCL_Pin, 400000); // 400 kHz

    // 4. Init battery (MAX17048)
    if (Battery_Init() != ERR_OK) {
        // Print via serial because logger might not be fully up yet
        Serial.println("Battery init failed, continuing anyway...");
    }

    // 5. Init pressure (ADS1115)
    if (Pressure_Init() != ERR_OK) {
        Serial.println("Pressure init failed, halting!");
        while(true) { vTaskDelay(portMAX_DELAY); }
    }

    // 6. Init acceleration (ADXL345)
    if (Acc_Init() != ERR_OK) {
        Serial.println("Accel init failed, continuing anyway...");
    }
   
    // 7. Init BLE (sideFlag => false => left, true => right)
    BLE_Init(g_sideFlag);

    // 8. Create tasks
    xTaskCreate(SensorTask, "SensorTask", 4096, NULL, 2, NULL);
    xTaskCreate(CommunicationTask, "CommTask", 4096, NULL, 2, NULL);

    LOG_INFO("Setup complete.");
}

void loop()
{
    // With FreeRTOS, do nothing in loop, tasks run in parallel
    vTaskDelay(portMAX_DELAY);
}
