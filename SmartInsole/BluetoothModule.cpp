#include "BluetoothModule.h"
#include "LoggerModule.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Advertising intervals
#define ADVERTISING_INTERVAL_MIN 0x20  // 20ms
#define ADVERTISING_INTERVAL_MAX 0x40  // 40ms

// Global variables
static BLEServer* pServer = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static BLEAdvertising* pAdvertising = nullptr;
static bool bleConnected = false;

// Message queue storage
static uint8_t MsgQueue[100][BLE_MSG_LENGTH];

// Watchdog timer variables
static unsigned long lastSuccessfulOperation = 0;
static const unsigned long BLE_WATCHDOG_TIMEOUT = 5000; // 5 seconds

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        bleConnected = true;
        LOG_INFO("BLE device connected");
        if (pAdvertising != nullptr) {
            pAdvertising->stop();
        }
    }

    void onDisconnect(BLEServer* pServer) override {
        bleConnected = false;
        LOG_INFO("BLE device disconnected");
        
        if (pTxCharacteristic != nullptr) {
            // Get the descriptor
            auto desc = pTxCharacteristic->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
            if (desc != nullptr) {
                uint8_t val[2] = {0, 0};
                desc->setValue(val, 2);
            }
        }
        
        // Restart advertising with a small delay
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay
        if (pAdvertising != nullptr) {
            pAdvertising->start();
            LOG_INFO("Advertising restarted");
        }
    }
};

void BLE_Watchdog(bool FlagSide) {
    if (millis() - lastSuccessfulOperation > BLE_WATCHDOG_TIMEOUT) {
        LOG_INFO("BLE watchdog triggered - restarting BLE");
        BLEDevice::deinit(true);
        delay(1000);
        BLE_Init(FlagSide);
        lastSuccessfulOperation = millis();
    }
}

bool BLE_Init(bool FlagSide)
{
    // 1. Choose name and UUIDs based on side flag
    const char* deviceName   = (FlagSide) ? INSOLE_NAME_RIGHT : INSOLE_NAME_LEFT;
    const char* serviceUUID  = (FlagSide) ? SERVICE_UUID_RIGHT : SERVICE_UUID_LEFT;
    const char* charUUID     = (FlagSide) ? CHARACTERISTIC_UUID_RIGHT : CHARACTERISTIC_UUID_LEFT;

    // 2. Initialize BLE
    BLEDevice::init(deviceName);

    // Increase TX power for better range
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

    // Optionally increase MTU to improve throughput
    BLEDevice::setMTU(50);

    // 3. Create BLE Server & set callbacks
    pServer = BLEDevice::createServer();
    if (pServer == nullptr) {
        LOG_ERROR("Failed to create BLE server");
        return false;
    }
    pServer->setCallbacks(new MyServerCallbacks());

    // 4. Create BLE Service
    BLEService* pService = pServer->createService(serviceUUID);
    if (pService == nullptr) {
        LOG_ERROR("Failed to create BLE service");
        return false;
    }

    // 5. Create a read+notify Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        charUUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    if (pTxCharacteristic == nullptr) {
        LOG_ERROR("Failed to create BLE characteristic");
        return false;
    }
    
    // Add a descriptor so the client can enable notifications
    pTxCharacteristic->addDescriptor(new BLE2902());

    // 6. Start the service
    pService->start();

    // 7. Start advertising
    pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising == nullptr) {
        LOG_ERROR("Failed to get advertising handle");
        return false;
    }
    
    // Configure advertising data
    BLEAdvertisementData advData;
    advData.setName(deviceName);
    advData.setCompleteServices(BLEUUID(serviceUUID));
    advData.setFlags(0x06); // BR/EDR not supported + LE General Discoverable Mode
    
    // Configure scan response data
    BLEAdvertisementData scanResponse;
    scanResponse.setName(deviceName);
    scanResponse.setManufacturerData("CorAlign");
    
    // Set the advertising data and scan response
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponse);
    
    // Configure advertising parameters
    pAdvertising->setMinInterval(ADVERTISING_INTERVAL_MIN);
    pAdvertising->setMaxInterval(ADVERTISING_INTERVAL_MAX);
    pAdvertising->setMinPreferred(0x06);  // 0x06 = 7.5ms connection interval min
    pAdvertising->setMaxPreferred(0x12);  // 0x12 = 22.5ms connection interval max
    
    // Start advertising
    pAdvertising->start();

    // Reset connection state
    bleConnected = false;

    LOG_INFO("BLE_Init complete. Device name: %s", deviceName);
    lastSuccessfulOperation = millis();  // Update watchdog timer
    return true;
}

bool BLE_SendByteArray(uint8_t* msg)
{
    if (bleConnected && pTxCharacteristic != nullptr) {
        // Get the descriptor
        auto desc = pTxCharacteristic->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        if (desc != nullptr) {
            // Get pointer to the descriptor value
            uint8_t* val = desc->getValue();
            if (val != nullptr && val[0] == 0x01) {  // Check if notifications are enabled
                pTxCharacteristic->setValue(msg, BLE_MSG_LENGTH);
                pTxCharacteristic->notify();
                lastSuccessfulOperation = millis();  // Update watchdog timer
                LOG_INFO("BLE_SendByteArray: %d bytes sent", BLE_MSG_LENGTH);
                return true;
            }
        }
    }
    return false;
}

void BLE_Test(void)
{
    // Example: initialize as "Right" side
    if (!BLE_Init(true)) {
        LOG_ERROR("BLE_Test: Init failed");
        return;
    }

    // Create a dummy 39-byte test message
    uint8_t testMsg[BLE_MSG_LENGTH];
    for (int i = 0; i < BLE_MSG_LENGTH; i++) {
        testMsg[i] = i;
    }

    // Send the test message
    BLE_SendByteArray(testMsg);

    LOG_INFO("BLE_Test complete. Use a BLE scanner to find '%s' and enable notifications",
             INSOLE_NAME_RIGHT);
}