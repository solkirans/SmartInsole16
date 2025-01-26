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

TaskHandle_t SensorTaskHandle = NULL;
TaskHandle_t CommunicationTaskHandle = NULL;
TaskHandle_t LoggerTaskHandle = NULL;
// 1) Sensor Task
void SensorTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_INTERVAL_MS);
    esp_task_wdt_add(NULL);
    for(;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
          // Read sensors
          if (BLE_GetNumOfSubscribers() > 0)
          {
            if (!testDeviceBLE)
            {
                Battery_Read();
                Acc_Read();
                Pressure_Read();
                //Lock data and pack it.
                if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
                    PackSensorData(sensor_data);
                    xSemaphoreGive(msgMutex);
                }
            }
            else
            {
                addDummyData(sensor_data);
            }
          }
          else
          {
            // If no BLE subscribers, clear the data
            clearSensorData(&sensor_data);
          }
        if (LOG_LEVEL_SELECTED >= LOGGER_LEVEL_DEBUG)
        {
            LoggerPrintLoopMessage(&sensor_data);
        }
        bool connstatus = Get_BLE_Connected_Status();
        uint8_t numSubscribers = BLE_GetNumOfSubscribers();
        if (connstatus && (numSubscribers > 0))
        {
            esp_task_wdt_reset();
        }
        

    }
}

// 2) Communication Task
void CommunicationTask(void* pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_INTERVAL_MS);
     esp_task_wdt_add(NULL);
    for(;;) {

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // Send via BLE
        if (BLE_GetNumOfSubscribers() > 0) {
            if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
                BLE_SendBuffer(&sensor_data);
                xSemaphoreGive(msgMutex);
            }
        }
        bool connstatus = Get_BLE_Connected_Status();
        uint8_t numSubscribers = BLE_GetNumOfSubscribers();
        if (LOG_LEVEL_SELECTED >= LOGGER_LEVEL_DEBUG)
        {
            LOG_DEBUG("Connection Status: %d", connstatus);
            LOG_DEBUG("Number of Subscribers: %d", numSubscribers);
        }
        if (connstatus && (numSubscribers > 0))
        {

            if (LOG_LEVEL_SELECTED >= LOGGER_LEVEL_DEBUG)
            {
                LOG_DEBUG("Watchdog fed by CommunicationTask");
            }
            esp_task_wdt_reset();
        }
    }

    
}

// 3) The usual Arduino setup
void setup()
{

    // 1. Logger init: let’s say we want INFO logs, with serial enabled
    LoggerInit();
    Serial.printf("Logger level set to %d\n\r", LOG_LEVEL_SELECTED);
    LOG_DEBUG("LoggerInit complete.");
    

    deviceResetReason();
    esp_task_wdt_init(WATCHDOG_PERIOD, true);

    // 2. Create logger task
    xTaskCreate(LoggerTask, "LoggerTask", LOGGER_TASK_STACK_SIZE, NULL, 3, &LoggerTaskHandle);
    LOG_DEBUG("LoggerTask setup complete.");


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
    xTaskCreate(SensorTask, "SensorTask", SENSOR_TASK_STACK_SIZE, NULL, 2, &SensorTaskHandle);
    LOG_DEBUG("SensorTask setup complete.");


    xTaskCreate(CommunicationTask, "CommTask", BLE_TASK_STACK_SIZE, NULL, 1, &CommunicationTaskHandle);
    LOG_DEBUG("CommunicationTask setup complete.");

    
    LOG_INFO("Setup complete.");

    LOG_DEBUG("SensorData size: %d bytes\n", sizeof(SensorData));
}

void loop()
{
    // With FreeRTOS, do nothing in loop, tasks run in parallel
    vTaskDelay(portMAX_DELAY);
}
