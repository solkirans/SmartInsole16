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

void BLE_Adv_Watchdog() {
    if((!bleConnected) && (!BLEDevice::getAdvertising()->isAdvertising())) {
        if(millis() - lastSuccessfulOperation > BLE_ADV_WATCHDOG_TIMEOUT) {
            LOG_WARN("Restarting advertising");
            BLEDevice::startAdvertising();
        }
    }
}

void BLE_Restart_Watchdog() {
    static uint32_t lastConnectionCheck = 0;
    
    if(!bleConnected && millis() - lastSuccessfulOperation > BLE_REBOOT_WATCHDOG_TIMEOUT) {
        LOG_ERROR("Critical BLE failure - restarting device");
        ESP.restart();
    }
}



// Optional watchdog to re-init BLE if idle too long
static void BLE_Init_Watchdog(bool FlagSide) {
    if (millis() - lastSuccessfulOperation > BLE_INIT_WATCHDOG_TIMEOUT) {
        LOG_ERROR("BLE watchdog triggered - restarting BLE");
        NimBLEDevice::deinit(true);
        pServer = nullptr;
        pTxCharacteristic = nullptr;
        pAdvertising = nullptr;
        BLE_Init(FlagSide);
        lastSuccessfulOperation = millis();
    }
}

void BLE_GeneralWathcdog(bool FlagSide) {
    LOG_DEBUG("lastCheck: %d", lastCheck);
    LOG_DEBUG("lastSuccessfulOperation: %d", lastSuccessfulOperation);
    if(millis() - lastCheck > BLE_WATCHDOG_PERIOD) 
    { // Run every 1 second
        lastCheck = millis();
        LOG_DEBUG("Watchdog check");
        BLE_Adv_Watchdog();
        BLE_Init_Watchdog(FlagSide);
        BLE_Restart_Watchdog();

    }
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
    advData.setName(deviceName);
    advData.setCompleteServices(NimBLEUUID(serviceUUID));
    advData.setFlags(0x06); // BR/EDR not supported + LE General Discoverable Mode

    // Configure scan response data
    NimBLEAdvertisementData scanResponse;
    scanResponse.setName(deviceName);
    // Use proper manufacturer ID + data format
    uint8_t mfgData[] = {0x01, 0x02, 'C','o','r','A','l','i','g','n'};
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


    // Set the value and notify
    pTxCharacteristic->setValue((uint8_t*)&sensor_msg, sizeof(SensorData));
    bool success = pTxCharacteristic->notify();
    
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
