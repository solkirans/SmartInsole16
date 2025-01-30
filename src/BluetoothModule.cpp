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

    // Set MTU to 50 bytes
    NimBLEDevice::setMTU(50);

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
    advData.setCompleteServices(NimBLEUUID(serviceUUID));
    uint8_t mfgData[] = {0x01, 0x02}; // Shortened manufacturer data
    scanResponse.setManufacturerData(mfgData, sizeof(mfgData));

    // Set the advertising and scan response data
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponse);

    // Configure advertising parameters
    pAdvertising->setMinInterval(ADVERTISING_INTERVAL_MIN);
    pAdvertising->setMaxInterval(ADVERTISING_INTERVAL_MAX);

    // Start advertising
    pAdvertising->start();

    bleConnected = false;

    LOG_INFO("BLE_Init complete. Device name: %s", deviceName);
    lastSuccessfulOperation = millis();  // Update watchdog timer
    return true;
}

bool processAndTransmitSensorData(const SensorData* data) {
  // 1. Create individual 2-byte arrays for each multi-byte field
  uint8_t battery_arr[1] = {data->battery}; // 1 byte

  uint8_t accel_x_arr[2];
  memcpy(accel_x_arr, &data->accel_x, sizeof(data->accel_x));

  uint8_t accel_y_arr[2];
  memcpy(accel_y_arr, &data->accel_y, sizeof(data->accel_y));

  uint8_t accel_z_arr[2];
  memcpy(accel_z_arr, &data->accel_z, sizeof(data->accel_z));

  uint8_t pressure_arr[16][2]; // 16 pressure values × 2 bytes
  for(int i=0; i<16; i++) {
    memcpy(pressure_arr[i], &data->pressure[i], sizeof(uint16_t));
  }



  // 3. Concatenate all arrays into one buffer
  uint8_t final_buffer[39];
  int offset = 0;

  // Battery (1 byte)
  memcpy(final_buffer + offset, battery_arr, 1);
  offset += 1;

  // Accelerations (2 bytes each)
  memcpy(final_buffer + offset, accel_x_arr, 2);
  offset += 2;
  memcpy(final_buffer + offset, accel_y_arr, 2);
  offset += 2;
  memcpy(final_buffer + offset, accel_z_arr, 2);
  offset += 2;

  // Pressures (2 bytes each)
  for(int i=0; i<16; i++) {
    memcpy(final_buffer + offset, pressure_arr[i], 2);
    offset += 2;
  }

  // 4. Print concatenated buffer
  char hex_str[3*39 + 1] = {0};
  for(int i=0; i<39; i++) {
    snprintf(hex_str + strlen(hex_str), sizeof(hex_str) - strlen(hex_str), "%02X ", final_buffer[i]);
  }
    if (LOG_LEVEL_SELECTED >= LOGGER_LEVEL_DEBUG)
    {
        // 2. Print individual components
        LOG_DEBUG("Battery: %02X", battery_arr[0]);

        LOG_DEBUG("Accel X: %02X %02X", accel_x_arr[0], accel_x_arr[1]);
        LOG_DEBUG("Accel Y: %02X %02X", accel_y_arr[0], accel_y_arr[1]);
        LOG_DEBUG("Accel Z: %02X %02X", accel_z_arr[0], accel_z_arr[1]);

        for(int i=0; i<16; i++) {
            LOG_DEBUG("Pressure[%d]: %02X %02X", i, pressure_arr[i][0], pressure_arr[i][1]);
        }
        LOG_DEBUG("Concatenated Buffer: %s", hex_str);
    }


  // 5. Transmit via BLE
  pTxCharacteristic->setValue(final_buffer, 39);
  return pTxCharacteristic->notify();
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


    // 3. Send the buffer via BLE
    bool success = processAndTransmitSensorData(sensor_msg);
    
    if (success) {
        lastSuccessfulOperation = millis();  // Update watchdog timer
        anySent = true;
    } else {
        LOG_WARN("BLE_SendByteArray: failed to notify");
        // If send fails, break the loop
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
