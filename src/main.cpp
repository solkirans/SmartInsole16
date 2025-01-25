#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "LoggerModule.h"
#include "PressureModule.h"
#include "AccModule.h"
#include "UtilitiesModule.h"
#include "BluetoothModule.h"
#include "CommonTypes.h"

// Globals

static unsigned long s_lastTaskTime = 0; // for watchdog

//Use a mutex to protect access to msg to prevent concurrent modifications
static SemaphoreHandle_t msgMutex = xSemaphoreCreateMutex();

SensorData sensor_data;
// Function to pack sensor data into BLE message
void PackSensorData() {
  
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
    memset(data, 0, sizeof(SensorData));
}

// 1) Sensor Task
void SensorTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_INTERVAL_MS);
    
    for(;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        BLE_GeneralWathcdog(g_sideFlag);
        // Watchdog check
        unsigned long now = millis();
        if ((now - s_lastTaskTime) > WATCHDOG_PERIOD_MS && s_lastTaskTime != 0) {
            LOG_ERROR("Watchdog triggered, system restarting...");
            ESP.restart();
        }
        s_lastTaskTime = now;
          // Read sensors
          if ((!testDeviceBLE) && (BLE_GetNumOfSubscribers() > 0))
          {
            Battery_Read();
            Acc_Read();
            Pressure_Read();

            //Lock data and pack it.
            if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
                PackSensorData();
                xSemaphoreGive(msgMutex);
            }
        
          }
          else
          {
            // If no BLE subscribers, clear the data
            clearSensorData(&sensor_data);
          }

        LoggerPrintLoopMessage(&sensor_data);

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
        //if (BLE_GetNumOfSubscribers() > 0) {
            if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
                BLE_SendBuffer(&sensor_data);
                xSemaphoreGive(msgMutex);
            }
        //}
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
    if (!testDeviceBLE)
    {
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

    }
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
