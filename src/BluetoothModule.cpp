#include "BluetoothModule.h"
#include "LoggerModule.h"

// Use NimBLE-Arduino library
#include "NimBLEDevice.h"

// If you want to set ESP32 TX power directly:
#include "esp_bt.h"



// Global variables
static NimBLEServer* pServer                   = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLEAdvertising* pAdvertising         = nullptr;
static bool bleConnected                       = false;

// Track subscription count
static volatile uint8_t numSubscribers = 0;

// Watchdog timer variables
static unsigned long lastSuccessfulOperation   = 0;
uint32_t lastCheck = 0;

// Add at the top with other static variables
static uint32_t lastTransmitTime = 0;

// Add a flag to track transmission status
static bool lastTransmissionSuccessful = true;



class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        bleConnected = true;
        LOG_INFO("BLE device connected");
        lastSuccessfulOperation = millis();

        if (pAdvertising) {
            pAdvertising->stop();
        }
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
        bleConnected = false;
        LOG_INFO("BLE device disconnected");
        if (pAdvertising) {
            pAdvertising->start();
            LOG_INFO("Advertising restarted");
        }
    }
    // Inside your NimBLE server callback:
    void onMTUChange(uint16_t MTU) {
        LOG_INFO("Negotiated MTU: %d\n", MTU);
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override  {
        if (!pTxCharacteristic) {
        LOG_ERROR("pTxCharacteristic is null!");
        return;
        }
        // subValue = 0: Unsubscribed
        // subValue = 1: Subscribed to notifications
        // subValue = 2: Subscribed to indications

        if (subValue != 0) {
            numSubscribers++;
        } else {
            numSubscribers--;
        }
        LOG_INFO("onSubscribe Called! %d active subscribers", numSubscribers);
        if (pCharacteristic->getUUID().equals(pTxCharacteristic->getUUID())) {
            LOG_INFO("Client %s notifications.", subValue ? "subscribed to" : "unsubscribed from");
        }
    }
};


bool Get_BLE_Connected_Status(void)
{
    return bleConnected;
}


uint8_t BLE_GetNumOfSubscribers(void)
{
    return numSubscribers;
}

bool BLE_Init(bool FlagSide)
{
    // 1. Choose name and UUIDs based on side flag
    const char* deviceName   = (FlagSide) ? INSOLE_NAME_RIGHT : INSOLE_NAME_LEFT;
    const char* serviceUUID  = (FlagSide) ? SERVICE_UUID_RIGHT : SERVICE_UUID_LEFT;
    const char* charUUID     = (FlagSide) ? CHARACTERISTIC_UUID_RIGHT : CHARACTERISTIC_UUID_LEFT;
    

    // 2. Initialize NimBLE
    NimBLEDevice::deinit(true); // Cleanup before re-initializing
    pServer = nullptr;
    pTxCharacteristic = nullptr;
    pAdvertising = nullptr;
    NimBLEDevice::init(deviceName);

    // (Optional) Set TX power for better range
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, BLE_TX_POWER);

    // Increase MTU size for better throughput
    NimBLEDevice::setMTU(100);  // Increase MTU size to 100 bytes

    // 3. Create BLE Server & set callbacks
    pServer = NimBLEDevice::createServer();
    if (!pServer) {
        LOG_ERROR("Failed to create BLE server");
        NimBLEDevice::deinit(true); // Explicit cleanup
        pServer = nullptr;
        pTxCharacteristic = nullptr;
        pAdvertising = nullptr;
        return false;
    }
    pServer->setCallbacks(new MyServerCallbacks());




    // 4. Create BLE Service
    NimBLEService* pService = pServer->createService(serviceUUID);
    if (!pService) {
        LOG_ERROR("Failed to create BLE service");
        return false;
    }

    // 5. Create a read+notify Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        charUUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    if (!pTxCharacteristic) {
        LOG_ERROR("Failed to create BLE characteristic");
        return false;
    }
    // start  callback
    pTxCharacteristic->setCallbacks(new CharacteristicCallbacks());


    // Add CCCD descriptor explicitly
    NimBLEDescriptor* cccd = pTxCharacteristic->createDescriptor(
        "2902",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    if (!cccd) {
        LOG_ERROR("Failed to create CCCD descriptor");
    }


    // 6. Start the service
    pService->start();

    // 7. Start advertising
    pAdvertising = NimBLEDevice::getAdvertising();
    if (!pAdvertising) {
        LOG_ERROR("Failed to get advertising handle");
        return false;
    }

    // Configure advertising data
    NimBLEAdvertisementData advData;
    advData.setFlags(0x06); // BR/EDR not supported + LE General Discoverable Mode
    advData.setName(deviceName);
    
    // Configure scan response data
    NimBLEAdvertisementData scanResponse;
    scanResponse.setCompleteServices(NimBLEUUID(serviceUUID));
    uint8_t mfgData[] = {0x01, 0x02}; // Shortened manufacturer data
    scanResponse.setManufacturerData(mfgData, sizeof(mfgData));

    // Set the advertising and scan response data
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponse);

    // Configure advertising parameters
    pAdvertising->setMinInterval(ADVERTISING_INTERVAL_MIN);
    pAdvertising->setMaxInterval(ADVERTISING_INTERVAL_MAX);

    // Optionally, use preferred intervals if needed
    // pAdvertising->setMinInterval(ADVERTISING_INTERVAL_MIN_PREFERRED);
    // pAdvertising->setMaxInterval(ADVERTISING_INTERVAL_MAX_PREFERRED);

    // Start advertising
    pAdvertising->start();

    bleConnected = false;

    LOG_INFO("BLE_Init complete. Device name: %s", deviceName);
    lastSuccessfulOperation = millis();  // Update watchdog timer
    return true;
}

bool processAndTransmitSensorData(const SensorData* data) {
   static TickType_t lastTransmitTick = 0;
   TickType_t currentTick = xTaskGetTickCount();
   
   // Convert ms to ticks for comparison
   const TickType_t minIntervalTicks = pdMS_TO_TICKS(BLE_TX_INTERVAL_MS);
   
   if ((currentTick - lastTransmitTick) < minIntervalTicks) {
     return false;
   }

   // Create buffer with exact size of SensorData
   uint8_t final_buffer[sizeof(SensorData)];
   int offset = 0;

   // Copy each field explicitly in the order defined in the struct
   // 1. Timestamp (4 bytes)
   memcpy(final_buffer + offset, &data->timestamp, sizeof(uint32_t));
   offset += sizeof(uint32_t);

   // 2. Battery (1 byte)
   final_buffer[offset] = data->battery;
   offset += sizeof(uint8_t);

   // 3. Accelerometer X (2 bytes)
   memcpy(final_buffer + offset, &data->accel_x, sizeof(int16_t));
   offset += sizeof(int16_t);

   // 4. Accelerometer Y (2 bytes)
   memcpy(final_buffer + offset, &data->accel_y, sizeof(int16_t));
   offset += sizeof(int16_t);

   // 5. Accelerometer Z (2 bytes)
   memcpy(final_buffer + offset, &data->accel_z, sizeof(int16_t));
   offset += sizeof(int16_t);

   // 6. Pressure array
   memcpy(final_buffer + offset, data->pressure, sizeof(data->pressure));
   offset += sizeof(data->pressure);

   // Verify total size matches struct size
   if (offset != sizeof(SensorData)) {
       LOG_ERROR("Buffer size mismatch: %d != %d", offset, sizeof(SensorData));
       return false;
   }

   // Set the value and notify
   pTxCharacteristic->setValue(final_buffer, sizeof(SensorData));
   bool success = pTxCharacteristic->notify();
   
   if (success) {
     lastTransmitTick = currentTick;
     esp_task_wdt_reset();
   }
   return success;
}

bool BLE_SendBuffer(SensorData* sensor_msg)
{
    // Try to send all buffered data
    bool anySent = false;
    LOG_DEBUG("Num of Subcribers: %d", numSubscribers);
    
    if (!pTxCharacteristic || !pServer) {
        LOG_ERROR("BLE characteristic/server not initialized");
        return false;
    }

    // Check if any client is connected
    if (pServer->getConnectedCount() == 0) {
        LOG_ERROR("No BLE clients connected");
        return false;
    }

    // Discard data if BLE is not ready
    if (!bleConnected || numSubscribers == 0) {
        LOG_WARN("BLE not ready, discarding data");
        return false;
    }

    // Send the buffer via BLE
    bool success = processAndTransmitSensorData(sensor_msg);
    
    if (success) {
        lastSuccessfulOperation = millis();  // Update watchdog timer
        anySent = true;
        esp_task_wdt_reset();  // Reset watchdog after successful send
    }
    
    return anySent;
}

// Modified BLE_Test to demonstrate buffer functionality
void BLE_Test(void)
{
    // Initialize BLE (right side)
    if (!BLE_Init(true)) {
        LOG_ERROR("BLE_Init failed");
        return;
    }

    // Create a dummy sensor message
    SensorData sensor_msg;
    sensor_msg.battery = 100;
    sensor_msg.accel_x = 123;
    sensor_msg.accel_y = 456;
    sensor_msg.accel_z = 789;
    for (int i = 0; i < 16; i++) {
        sensor_msg.pressure[i] = 1000 + i;
    }

    // Send the message
    if (BLE_SendBuffer(&sensor_msg)) {
        LOG_INFO("BLE_SendBuffer: success");
    } else {
        LOG_ERROR("BLE_SendBuffer: failed");
    }
}
