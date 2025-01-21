#include "BluetoothModule.h"
#include "LoggerModule.h"

// Use NimBLE-Arduino library
#include "NimBLEDevice.h"

// (Optional) If you want to set ESP32 TX power directly:
#include "esp_bt.h"

// ------------------------------
// Advertising intervals
// ------------------------------
#define ADVERTISING_INTERVAL_MIN 0x50  // 0x50 * 0.625ms = 31.25ms
#define ADVERTISING_INTERVAL_MAX 0x100 // 0x100 * 0.625ms = 160ms

// Global variables
static NimBLEServer* pServer                 = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLEAdvertising* pAdvertising       = nullptr;
static bool bleConnected                     = false;

// Watchdog timer variables
static unsigned long lastSuccessfulOperation = 0;
static const unsigned long BLE_WATCHDOG_TIMEOUT = 5000; // 5 seconds

class MyServerCallbacks : public NimBLEServerCallbacks {
public:
    virtual bool onConnect(NimBLEServer* pServer) {
        bleConnected = true;
        LOG_INFO("BLE device connected");

        if (pAdvertising) {
            pAdvertising->stop();
        }
        return true;
    }

    virtual void onDisconnect(NimBLEServer* pServer) {
        bleConnected = false;
        LOG_INFO("BLE device disconnected");

        // Don't manually clear the 0x2902 descriptor; iOS clients handle it
        vTaskDelay(pdMS_TO_TICKS(100));
        if (pAdvertising) {
            pAdvertising->start();
            LOG_INFO("Advertising restarted");
        }
    }
};

// If you still want a watchdog re-init:
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

    // Remove/disable custom MTU (iOS can be finicky)
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
    advData.setFlags(0x06); // BR/EDR not supported + LE General Discoverable

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

    // Reset connection state
    bleConnected = false;

    LOG_INFO("BLE_Init complete. Device name: %s", deviceName);
    lastSuccessfulOperation = millis();  // Update watchdog timer
    return true;
}

bool BLE_SendByteArray(uint8_t* msg)
{
    // If connected and the characteristic is valid...
    if (bleConnected && pTxCharacteristic) {
        // Check if notifications are enabled by reading the 0x2902 descriptor
        NimBLEDescriptor* desc = pTxCharacteristic->getDescriptorByUUID("2902");
        if (desc) {
            const uint8_t* val = desc->getValue();
            // bit 0 of val[0] indicates if Notify is enabled
            if (val != nullptr && (val[0] & 0x01)) {
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
