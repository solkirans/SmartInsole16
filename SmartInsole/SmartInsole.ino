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
static bool g_sideFlag = 0;  // 0 => left, 1 => right
static unsigned long s_lastTaskTime = 0; // for watchdog
bool FlagReadingDone = false;
static unsigned long lastPrintTime = 0;
static  uint8_t msg[BLE_MSG_LENGTH];

// Function to pack sensor data into BLE message
void PackSensorData(uint8_t* msg) {
  
    // Prepare the message and add it to the BLE msg queue
    // Build 39-byte payload:
    //   [0]   = BatteryVoltage (1 byte)
    //   [1..6] = Acc_Array (3× int16 => 6 bytes)
    //   [7..38] = Pressure_Array (16× uint16 => 32 bytes)
    if (!msg) return;
    
    int index = 0;
    
    // Pack battery data (1 byte)
    msg[index++] = BatteryVoltage;
    
    // Pack accelerometer data (6 bytes - 3 int16_t values)
    for (int i = 0; i < 3; i++) {
        msg[index++] = (Acc_Array[i] >> 8) & 0xFF;  // High byte
        msg[index++] = Acc_Array[i] & 0xFF;         // Low byte
    }
    
    // Pack pressure data (32 bytes - 16 uint16_t values)
    for (int i = 0; i < 16; i++) {
        msg[index++] = (Pressure_Array[i] >> 8) & 0xFF;  // High byte
        msg[index++] = Pressure_Array[i] & 0xFF;         // Low byte
    }
}

// 0) Print function
void printLoopMessage(uint8_t* msg) {
    unsigned long currentTime = millis();
    
    // Check if enough time has passed since last print
    if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = currentTime;  // Update the last print time
        
        // Create debug string
        String dbg;
        for (int i = 0; i < BLE_MSG_LENGTH; i++) {
            dbg += String(msg[i], HEX);
            dbg += " ";
        }
        LOG_INFO("TxMsg: %s", dbg.c_str());
    }
}


// 1) Sensor Task
void SensorTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_INTERVAL_MS);
    
    for(;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

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
        PackSensorData(msg);
        printLoopMessage(msg);

    }
}

// 2) Communication Task
void CommunicationTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_INTERVAL_MS);

    for(;;) {

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // Send via BLE
        BLE_SendBuffer(msg);
        vTaskDelay(BLE_MSG_DELAY); // give BLE stack a moment to send

    }
}

// 3) The usual Arduino setup
void setup()
{
    // 1. Logger init: let’s say we want INFO logs, with serial enabled
    LoggerInit();
    LOG_DEBUG("LoggerInit complete.");


    // 2. Create logger task
    xTaskCreate(LoggerTask, "LoggerTask", LOGGER_TASK_STACK_SIZE, NULL, 3, NULL);
    LOG_DEBUG("LoggerTask complete.");



    // 3. Initialize I2C
    Wire.begin(I2C_SDA_Pin, I2C_SCL_Pin, 400000); // 400 kHz
    LOG_DEBUG("Wire.begin complete.");
    i2cScanner();

    // 4. Init battery (MAX17048)
    if (Battery_Init() != ERR_OK) {
        // Print via serial because logger might not be fully up yet
        LOG_ERROR("Battery init failed, continuing anyway...");
    }
    LOG_DEBUG("Battery_Init complete.");


    // 5. Init pressure (ADS1115)
    if (Pressure_Init() != ERR_OK) {
        LOG_ERROR("Pressure init failed, halting!");
        while(true) { vTaskDelay(portMAX_DELAY); }
    }
    LOG_DEBUG("Pressure_Init complete.");


    // 6. Init acceleration (ADXL345)
    if (Acc_Init() != ERR_OK) {
        LOG_ERROR("Accel init failed, continuing anyway...");
    }
    LOG_DEBUG("Acc_Init complete.");


    // 7. Init BLE (sideFlag => false => left, true => right)
    BLE_Init(g_sideFlag);
    LOG_DEBUG("BLE_Init complete.");


    // 8. Create tasks
    xTaskCreate(SensorTask, "SensorTask", SENSOR_TASK_STACK_SIZE, NULL, 2, NULL);
    LOG_DEBUG("SensorTask complete.");


    xTaskCreate(CommunicationTask, "CommTask", BLE_TASK_STACK_SIZE, NULL, 1, NULL);
    LOG_DEBUG("CommunicationTask complete.");

    
    LOG_INFO("Setup complete.");
}

void loop()
{
    // With FreeRTOS, do nothing in loop, tasks run in parallel
    vTaskDelay(portMAX_DELAY);
}
