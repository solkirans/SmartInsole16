#include "BluetoothModule.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// We assume you have a LoggerModule or similar for debug logs.
// If not, replace LOG_INFO/LOG_ERROR with Serial.print as needed.
#include "LoggerModule.h"

// -----------------------------------------------------------------------------
// Local Variables
// -----------------------------------------------------------------------------

// A 2D array for potential message queue storage (39 columns × 100 rows).
// Even if you're sending immediately, you can keep this as a buffer if needed.
static uint8_t MsgQueue[100][BLE_MSG_LENGTH];

// BLE objects
static BLEServer*         pServer          = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static BLEAdvertising*    pAdvertising     = nullptr;

// Track connection state
static bool bleConnected = false;


// Add at the top with other globals
static unsigned long lastSuccessfulOperation = 0;
static const unsigned long BLE_WATCHDOG_TIMEOUT = 5000; // 5 seconds

// Add this function to your code
void BLE_Watchdog(bool FlagSide) {
    if (millis() - lastSuccessfulOperation > BLE_WATCHDOG_TIMEOUT) {
        LOG_INFO("BLE watchdog triggered - restarting BLE");
        BLEDevice::deinit(true);
        delay(1000);
        BLE_Init(sideFlag);
        lastSuccessfulOperation = millis();
    }
}
// -----------------------------------------------------------------------------
// BLE Callbacks to monitor connections
// -----------------------------------------------------------------------------
// Modify MyServerCallbacks to include reconnection logic
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        bleConnected = true;
        LOG_INFO("BLE device connected");
        // Stop advertising when connected
        pAdvertising->stop();
    }

    void onDisconnect(BLEServer* pServer) override {
        bleConnected = false;
        LOG_INFO("BLE device disconnected");
        
        // Reset the characteristic for the next connection
        pTxCharacteristic->clearSubscribe();
        
        // Restart advertising with a small delay
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay
        pAdvertising->start();
        LOG_INFO("Advertising restarted");
    }
};

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------

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
    // (Phones often negotiate down, but let's request a bigger MTU)
    BLEDevice::setMTU(50);

    // 3. Create BLE Server & set callbacks
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // 4. Create BLE Service
    BLEService* pService = pServer->createService(serviceUUID);

    // 5. Create a read+notify Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        charUUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    // Add a descriptor so the client can enable notifications
    pTxCharacteristic->addDescriptor(new BLE2902());

    // 6. Start the service
    pService->start();

    // 7. Start advertising
    pAdvertising = BLEDevice::getAdvertising();
    // Set advertising parameters
    esp_ble_gap_set_ext_adv_params_all_phys(); // Enable extended advertising
    // Configure advertising data
    BLEAdvertisementData advData;
    advData.setName(deviceName);
    advData.setCompleteServices(BLEUUID(serviceUUID));
    advData.setFlags(0x06); // BR/EDR not supported + LE General Discoverable Mode
    // Configure scan response data
    BLEAdvertisementData scanResponse;
    scanResponse.setName(deviceName); // Include name in scan response too
    scanResponse.setManufacturerData("CorAlign"); // Optional: Add manufacturer data
    // Set the advertising data and scan response
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponse);
    // Configure advertising parameters
    pAdvertising->setMinInterval(ADVERTISING_INTERVAL_MIN);
    pAdvertising->setMaxInterval(ADVERTISING_INTERVAL_MAX);
    pAdvertising->setMinPreferred(0x06);  // 0x06 = 7.5ms connection interval min
    pAdvertising->setMaxPreferred(0x12);  // 0x12 = 22.5ms connection interval max
    pAdvertising->start();

    bleConnected = false;  // reset connection state

    LOG_INFO("BLE_Init complete. Device name: %s", deviceName);
    return true;
}

bool BLE_SendByteArray(uint8_t* msg)
{
    if (bleConnected) {
      BLEDescriptor* pDesc = pTxCharacteristic->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      if (pDesc) {
        uint8_t* val = pDesc->getValue();
        if (val && val[0] == 1) {
          // OK to notify
          pTxCharacteristic->setValue(msg, BLE_MSG_LENGTH);
          pTxCharacteristic->notify();
        }
      }
    }
    LOG_INFO("BLE_SendByteArray: 39 bytes sent");
    return true;
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

    // Immediately send
    BLE_SendByteArray(testMsg);

    LOG_INFO("BLE_Test complete. Use an iPhone BLE scanner (e.g. nRF Connect) "
             "to find 'Insole Right' and enable notifications on the characteristic.");
}
