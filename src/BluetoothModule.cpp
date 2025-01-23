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

// Watchdog timer variables
static unsigned long lastSuccessfulOperation   = 0;
static const unsigned long BLE_WATCHDOG_TIMEOUT = 5000; // 5 seconds

class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        bleConnected = true;
        LOG_INFO("BLE device connected");

        if (pAdvertising) {
            pAdvertising->stop();
        }
    }

    void onDisconnect(NimBLEServer* pServer) {
        bleConnected = false;
        LOG_INFO("BLE device disconnected");

        vTaskDelay(pdMS_TO_TICKS(100));
        if (pAdvertising) {
            pAdvertising->start();
            LOG_INFO("Advertising restarted");
        }
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
        if (pCharacteristic->getUUID().equals(pTxCharacteristic->getUUID())) {
            LOG_INFO("Client %s notifications.", subValue ? "subscribed to" : "unsubscribed from");
        }
    }
};

// Optional watchdog to re-init BLE if idle too long
static void BLE_Watchdog(bool FlagSide) {
    if (millis() - lastSuccessfulOperation > BLE_WATCHDOG_TIMEOUT) {
        LOG_INFO("BLE watchdog triggered - restarting BLE");

        NimBLEDevice::deinit(true);
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

    // 2. Initialize NimBLE
    NimBLEDevice::init(deviceName);

    // (Optional) Set TX power for better range
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

    // Set MTU to 50 bytes
    NimBLEDevice::setMTU(50);

    // 3. Create BLE Server & set callbacks
    pServer = NimBLEDevice::createServer();
    if (!pServer) {
        LOG_ERROR("Failed to create BLE server");
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
    // Add a descriptor for notifications
    pTxCharacteristic->createDescriptor(
        "2902",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );

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
    scanResponse.setManufacturerData("CorAlign");

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

bool BLE_SendBuffer(uint8_t* data)
{
// Try to send all buffered data
    bool anySent = false;


    // Set the value and notify
    pTxCharacteristic->setValue(data, BLE_MSG_LENGTH);
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
    // Example: initialize as "Right" side
    if (!BLE_Init(true)) {
        LOG_ERROR("BLE_Test: Init failed");
        return;
    }

    // Create multiple test messages
    uint8_t testMsg[BLE_MSG_LENGTH];
    
    // Send multiple test messages to demonstrate buffer
    for (int j = 0; j < 5; j++) {
        // Fill test message with incrementing values
        for (int i = 0; i < BLE_MSG_LENGTH; i++) {
            testMsg[i] = (i + j) % 256;
        }
        
        // Send the test message
        BLE_SendBuffer(testMsg);
        
        // Add delay between test messages
        delay(100);
    }

    LOG_INFO("BLE_Test complete. Use a BLE scanner to find '%s' and enable notifications",
             INSOLE_NAME_RIGHT);
}