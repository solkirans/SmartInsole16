#include <Arduino.h>
#include <freertos/task.h>
#include <Wire.h>
#include "Config.h"
#include "LoggerModule.h"
#include "PressureModule.h"
#include "AccModule.h"
#include "UtilitiesModule.h"
#include "BluetoothModule.h"
#include "CommonTypes.h"

// Global data protected by mutex
static SemaphoreHandle_t msgMutex = nullptr;
static SensorData sensor_data = {0};

// Task handles
static TaskHandle_t SensorTaskHandle = nullptr;
static TaskHandle_t CommunicationTaskHandle = nullptr;
static TaskHandle_t LoggerTaskHandle = nullptr;

// Function prototypes
static void initializeTasks(void);
static void initializeHardware(void);
static void initializeSensors(void);

// Sensor Task: Reads sensors and updates global data
static void SensorTask(void* pvParam)
{
    // Set task to core 1 and increase priority
    vTaskSetThreadLocalStoragePointer(NULL, 0, (void*)xTaskGetCurrentTaskHandle());
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LOOP_INTERVAL_MS);
    uint32_t lastExecutionTime = 0;

    for(;;) {
        uint32_t startTime = millis();
        
        // Precise timing control
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        if (BLE_GetNumOfSubscribers() > 0) {
            if (!testDeviceBLE) {
                // Read all sensors atomically with timeout
                if (xSemaphoreTake(msgMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
                    Battery_Read();
                    Acc_Read();
                    Pressure_Read();
                    PackSensorData(sensor_data);
                    xSemaphoreGive(msgMutex);
                }
            } else {
                if (xSemaphoreTake(msgMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
                    addDummyData(sensor_data);
                    xSemaphoreGive(msgMutex);
                }
            }
        }
        
        // Monitor timing consistency
        uint32_t executionTime = millis() - startTime;
        if (executionTime > LOOP_INTERVAL_MS + SENSOR_TIMING_TOLERANCE_MS) {
            LOG_WARN("Sensor task exceeded timing tolerance: %dms", executionTime);
        }
    }
}

// Communication Task: Handles BLE transmission
static void CommunicationTask(void* pvParam)
{
    // Set task to core 0 and maximum priority
    vTaskSetThreadLocalStoragePointer(NULL, 0, (void*)xTaskGetCurrentTaskHandle());
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(BLE_TX_INTERVAL_MS);
    esp_task_wdt_add(NULL);
    
    static int recoveryAttempts = 0;
    static uint32_t lastStatusLog = 0;
    static uint32_t lastTransmitTime = 0;

    for(;;) {
        uint32_t startTime = millis();
        
        // Precise timing control
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        esp_task_wdt_reset();
        uint8_t numSubscribers = BLE_GetNumOfSubscribers();
        uint8_t connstatus = Get_BLE_Connected_Status();
        uint32_t currentTime = millis();
        if (connstatus)
        {
            if (numSubscribers) {
                
                if (xSemaphoreTake(msgMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
                    // Update timestamp just before sending
                    sensor_data.timestamp = currentTime;
                    bool success = BLE_SendBuffer(&sensor_data);
                    xSemaphoreGive(msgMutex);
                    
                    if (success) {
                        
                        // Monitor transmission timing
                        uint32_t transmitInterval = currentTime - lastTransmitTime;
                        if (abs((int32_t)transmitInterval - BLE_TX_INTERVAL_MS) > BLE_TX_TIMING_TOLERANCE_MS) {
                            LOG_WARN("BLE interval deviation: %dms", transmitInterval - BLE_TX_INTERVAL_MS);
                        }
                        
                        lastTransmitTime = currentTime;
                        recoveryAttempts = 0;
                    } else if (++recoveryAttempts > BLE_MAX_ERRORS_BEFORE_RECOVERY) {
                        LOG_ERROR("BLE connection lost, reinitializing...");
                        BLE_Init(g_sideFlag);
                        recoveryAttempts = 0;
                        lastTransmitTime = currentTime;
                    }
                }
            }
        }

        // Status logging
        if (currentTime - lastStatusLog >= PRINT_INTERVAL) {
            lastStatusLog = currentTime;
            LOG_INFO("BLE Status - Connected: %d, Subscribers: %d", connstatus, numSubscribers);
        }

        // Monitor timing consistency
        uint32_t executionTime = millis() - startTime;
        if (executionTime > BLE_TX_INTERVAL_MS + BLE_TX_TIMING_TOLERANCE_MS) {
            LOG_WARN("BLE task exceeded timing tolerance: %dms", executionTime);
        }
    }
}

static void initializeHardware(void)
{
    esp_task_wdt_init(WATCHDOG_PERIOD, true);
    Wire.begin(I2C_SDA_Pin, I2C_SCL_Pin, I2C_FREQUENCY);
    LoggerInit();
    LOG_INFO("Starting up... (Log Level: %d)", LOG_LEVEL_SELECTED);
}

static void initializeSensors(void)
{
    if (!testDeviceBLE) {
        uint8_t status = ERR_OK;
        status |= Battery_Init();
        status |= Pressure_Init();
        status |= Acc_Init();
        
        if (status != ERR_OK) {
            LOG_ERROR("Sensor initialization failed with status: 0x%02X", status);
        }
    }
}

static void initializeTasks(void)
{
    // Create mutex
    msgMutex = xSemaphoreCreateMutex();
    if (!msgMutex) {
        LOG_ERROR("Failed to create mutex");
        return;
    }

    // Create tasks with appropriate priorities and core pinning
    BaseType_t status = pdPASS;
    
    // Logger task on core 0
    status &= xTaskCreatePinnedToCore(
        LoggerTask, "Logger", 
        LOGGER_TASK_STACK_SIZE,
        NULL, TASK_PRIORITY_LOGGER,
        &LoggerTaskHandle,
        CORE_LOGGER
    );
    
    // Sensor task on core 1
    status &= xTaskCreatePinnedToCore(
        SensorTask, "Sensor",
        SENSOR_TASK_STACK_SIZE,
        NULL, TASK_PRIORITY_SENSOR,
        &SensorTaskHandle,
        CORE_SENSOR
    );
    
    // BLE task on core 0
    status &= xTaskCreatePinnedToCore(
        CommunicationTask, "BLE",
        BLE_TASK_STACK_SIZE,
        NULL, TASK_PRIORITY_BLE,
        &CommunicationTaskHandle,
        CORE_BLE
    );

    if (status != pdPASS) {
        LOG_ERROR("Failed to create one or more tasks");
    }
}

void setup()
{
    initializeHardware();
    initializeSensors();
    BLE_Init(g_sideFlag);
    initializeTasks();
    LOG_INFO("Setup complete");
}

void loop()
{
    vTaskDelete(NULL);
}
