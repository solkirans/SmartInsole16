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
static uint8_t MsgQueue[100][39];

// BLE objects
static BLEServer*         pServer          = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static BLEAdvertising*    pAdvertising     = nullptr;

// Track connection state
static bool bleConnected = false;

// -----------------------------------------------------------------------------
// BLE Callbacks to monitor connections
// -----------------------------------------------------------------------------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    bleConnected = true;
    LOG_INFO("BLE device connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    bleConnected = false;
    LOG_INFO("BLE device disconnected");
    // Resume advertising so another device can connect
    pAdvertising->start();
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
    // Advertise our service UUID
    pAdvertising->addServiceUUID(serviceUUID);
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
          pTxCharacteristic->setValue(msg, 39);
          pTxCharacteristic->notify();
        }
      }
    }
    vTaskDelay(1); // give BLE stack a moment to send
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
    uint8_t testMsg[39];
    for (int i = 0; i < 39; i++) {
        testMsg[i] = i;
    }

    // Immediately send
    BLE_SendByteArray(testMsg);

    LOG_INFO("BLE_Test complete. Use an iPhone BLE scanner (e.g. nRF Connect) "
             "to find 'Insole Right' and enable notifications on the characteristic.");
}
