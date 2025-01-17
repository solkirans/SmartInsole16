#include "Logger.h"
#include <stdint.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = nullptr;
BLEService *pAccelService = nullptr;
BLEService *pPressureService = nullptr;
BLECharacteristic *pAccelCharacteristic = nullptr;
BLECharacteristic *pPressureCharacteristic = nullptr;

void SetupLogger(void) {
    PrintText(LoggerDebugLevel_Debug, "SetupBL: Start");
    String DeviceName = "SmartInsole";
    DeviceName += (SideFlag == 0) ? "L" : "R";
    String resultString = "SetupBL: DeviceName: " + String(DeviceName.c_str());
    PrintText(LoggerDebugLevel_Debug, resultString);


    BLEDevice::init(DeviceName.c_str());
    PrintText(LoggerDebugLevel_Debug, "SetupBL: BLEDevice init done");

    BLEDevice::setMTU(BLE_MTU_Size);
    PrintText(LoggerDebugLevel_Debug, "SetupBL: setMTU done");

    pServer = BLEDevice::createServer();
    PrintText(LoggerDebugLevel_Debug, "SetupBL: createServer done");

    // Accelerometer Service
    pAccelService = pServer->createService(
        SideFlag == 0 ? UUID_ACCEL_SERVICE_LEFT : UUID_ACCEL_SERVICE_RIGHT);
    PrintText(LoggerDebugLevel_Debug, "SetupBL: createService done (pAccelService)");

    pAccelCharacteristic = pAccelService->createCharacteristic(
        SideFlag == 0 ? UUID_ACCEL_CHARACTERISTIC_LEFT : UUID_ACCEL_CHARACTERISTIC_RIGHT,
        BLECharacteristic::PROPERTY_NOTIFY);
    PrintText(LoggerDebugLevel_Debug, "SetupBL: createCharacteristic done (pAccelCharacteristic)");

    pAccelCharacteristic->addDescriptor(new BLE2902());
    PrintText(LoggerDebugLevel_Debug, "SetupBL: added BLE2902 descriptor");
    

    // Pressure Sensor Service
    pPressureService = pServer->createService(
        SideFlag == 0 ? UUID_PRESSURE_SERVICE_LEFT : UUID_PRESSURE_SERVICE_RIGHT);
    PrintText(LoggerDebugLevel_Debug, "SetupBL: createService done (pPressureService)");

    pPressureCharacteristic = pPressureService->createCharacteristic(
        SideFlag == 0 ? UUID_PRESSURE_CHARACTERISTIC_LEFT : UUID_PRESSURE_CHARACTERISTIC_RIGHT,
        BLECharacteristic::PROPERTY_NOTIFY);
    PrintText(LoggerDebugLevel_Debug, "SetupBL: createCharacteristic done (pPressureCharacteristic)");

    pPressureCharacteristic->addDescriptor(new BLE2902());
    PrintText(LoggerDebugLevel_Debug, "SetupBL: added BLE2902 descriptor (pPressureCharacteristic)");
    

    // Start Services
    pAccelService->start();
    PrintText(LoggerDebugLevel_Debug, "SetupBL: pAccelService->start() done");

    pPressureService->start();
    PrintText(LoggerDebugLevel_Debug, "SetupBL: pPressureService->start() done");

    // Advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    PrintText(LoggerDebugLevel_Debug, "SetupBL: gotAdvertising() done");

    pAdvertising->addServiceUUID(
        SideFlag == 0 ? UUID_ACCEL_SERVICE_LEFT : UUID_ACCEL_SERVICE_RIGHT);
    pAdvertising->addServiceUUID(
        SideFlag == 0 ? UUID_PRESSURE_SERVICE_LEFT : UUID_PRESSURE_SERVICE_RIGHT);
    PrintText(LoggerDebugLevel_Debug, "SetupBL: added service UUIDs");

    pAdvertising->start();
    PrintText(LoggerDebugLevel_Debug, "SetupBL: advertising started");
}

void sendPressureArray(uint16_t *ArrayValues, size_t lengthValues, uint32_t timestamp) {
    size_t lengthTransmit = lengthValues + 2;
    uint16_t ArrayTransmit[lengthTransmit];

    for (size_t i = 0; i < lengthValues; i++) {
        ArrayTransmit[i] = ArrayValues[i];
    }

    ArrayTransmit[lengthValues] = (timestamp >> 16) & 0xFFFF;
    ArrayTransmit[lengthValues + 1] = timestamp & 0xFFFF;

    if (pPressureCharacteristic != nullptr) {
        pPressureCharacteristic->setValue((uint8_t *)ArrayTransmit, lengthTransmit * sizeof(uint16_t));
        pPressureCharacteristic->notify();
        PrintText(LoggerDebugLevel_Debug, "sendPressureArray Done.");
    }
    
}

void sendAccelerometerArray(uint16_t *ArrayValues, size_t lengthValues, uint32_t timestamp) {
    size_t lengthTransmit = lengthValues + 2;
    uint16_t ArrayTransmit[lengthTransmit];

    for (size_t i = 0; i < lengthValues; i++) {
        ArrayTransmit[i] = ArrayValues[i];
    }

    ArrayTransmit[lengthValues] = (timestamp >> 16) & 0xFFFF;
    ArrayTransmit[lengthValues + 1] = timestamp & 0xFFFF;

    if (pAccelCharacteristic != nullptr) {
        pAccelCharacteristic->setValue((uint8_t *)ArrayTransmit, lengthTransmit * sizeof(uint16_t));
        pAccelCharacteristic->notify();
        PrintText(LoggerDebugLevel_Debug, "sendAccelerometerArray Done.");
    }
}

int PrintText(int DebugLevel, String OutputText) {
    int returnValue = PrintText_NA;

    if (DebugLevel >= LoggerDebugLevel) {
        if (Logger_Serial_Enabled) {
            if (Serial) {
                Serial.println(OutputText);
                returnValue = PrintText_Success;
            }
        }
    }

    return returnValue;
}

void testLogger() {
    uint16_t testPressureArray[] = {100, 200, 300, 400};
    uint16_t testAccelerometerArray[] = {500, 600, 700, 800};
    uint32_t testTimestamp = 0x12345678;

        // Test PrintText
    Serial.println("Testing PrintText...");
    int result = PrintText(LoggerDebugLevel_Info, "This is a test message for PrintText.");
    if (result == PrintText_Success) {
        Serial.println("PrintText test passed!");
    } else if (result == PrintText_BleFail) {
        Serial.println("PrintText test failed (BLE failure)!");
    } else if (result == PrintText_SerialFail) {
        Serial.println("PrintText test failed (Serial failure)!");
    } else if (result == PrintText_BleAndSerialFail) {
        Serial.println("PrintText test failed (BLE and Serial failure)!");
    } else {
        Serial.println("PrintText test returned unknown result!");
    }


    // Setup the logger
    SetupLogger();
    PrintText(LoggerDebugLevel_Debug, "Logger setup complete!");

    // Test sendPressureArray
    PrintText(LoggerDebugLevel_Debug, "Testing sendPressureArray...");
    sendPressureArray(testPressureArray, sizeof(testPressureArray) / sizeof(testPressureArray[0]), testTimestamp);
    PrintText(LoggerDebugLevel_Debug, "sendPressureArray test completed!");

    // Test sendAccelerometerArray
    PrintText(LoggerDebugLevel_Debug, "Testing sendAccelerometerArray...");
    sendAccelerometerArray(testAccelerometerArray, sizeof(testAccelerometerArray) / sizeof(testAccelerometerArray[0]), testTimestamp);
    PrintText(LoggerDebugLevel_Debug, "sendAccelerometerArray test completed!");


}



