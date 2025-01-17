#include "AccelerometerModule.h"
#include "Logger.h"
// Define accelerometer global variables
uint16_t AccelerometerValues[ADXL345_DataLength] = {0,0,0};
uint32_t AccelerometerTimestamp = 0;

// ADXL345 I2C Address
#define ADXL345_I2C_ADDRESS 0x53

// Accelerometer setup function
bool AccelerometerSetup() {
    Wire.begin();

    // Set BW_RATE register (0x2C) to ADXL345_DataRate_100Hz
    Wire.beginTransmission(ADXL345_I2C_ADDRESS);
    Wire.write(0x2C);
    Wire.write(ADXL345_DataRate_200Hz);
    PrintText(LoggerDebugLevel_Debug, "ADXL345_DataRate write done.");
    if (Wire.endTransmission() != 0) {
        PrintText(LoggerDebugLevel_Error, "ADXL345_DataRate write ERROR.");
        return true; // Error during setup
    }

    // Set DATA_FORMAT register (0x31) to ADXL345_Range
    Wire.beginTransmission(ADXL345_I2C_ADDRESS);
    Wire.write(0x31);
    Wire.write(ADXL345_Range);
    PrintText(LoggerDebugLevel_Debug, "ADXL345_Range write done.");
    if (Wire.endTransmission() != 0) {
        PrintText(LoggerDebugLevel_Error, "ADXL345_Range write ERROR.");
        return true; // Error during setup
    }

    // Set POWER_CTL register (0x2D) to ADXL345_POWER_CTL_Measurement
    Wire.beginTransmission(ADXL345_I2C_ADDRESS);
    Wire.write(0x2D);
    Wire.write(ADXL345_POWER_CTL_Measurement);
    PrintText(LoggerDebugLevel_Debug, "ADXL345_POWER_CTL_Measurement write done.");
    if (Wire.endTransmission() != 0) {
        PrintText(LoggerDebugLevel_Error, "ADXL345_POWER_CTL_Measurement write ERROR.");
        return true; // Error during setup
    }

    // Set FIFO_CTL register (0x38) to ADXL345_FIFO_CTL_Stream
    Wire.beginTransmission(ADXL345_I2C_ADDRESS);
    Wire.write(0x38);
    Wire.write(ADXL345_FIFO_CTL_Stream);
    PrintText(LoggerDebugLevel_Debug, "ADXL345_FIFO_CTL_Stream write done.");
    if (Wire.endTransmission() != 0) {
        PrintText(LoggerDebugLevel_Error, "ADXL345_FIFO_CTL_Stream write ERROR.");
        return true; // Error during setup
    }
    PrintText(LoggerDebugLevel_Debug, "ADXL345 setup done.");
    return false; // Setup successful
}

// Accelerometer read function
bool AccelerometerRead() {
    // We'll read 3 axes (X, Y, Z) from the ADXL345,
    // so we have a temporary buffer for the raw 16-bit values.
    int16_t ADXLArray[ADXL345_DataLength] = {0};

    // 1) Write the register address we want to read from (0x32 = DATAX0 on ADXL345).
    //    This is done by starting an I2C "write" transaction just to set the device's
    //    internal pointer to 0x32.
    Wire.beginTransmission(ADXL345_I2C_ADDRESS);
    PrintText(LoggerDebugLevel_Debug, "ADXL345 begin");
    Wire.write(0x32); // The first register address to read (DATAX0).
    PrintText(LoggerDebugLevel_Debug, "ADXL345 write 0x32");

    // 2) endTransmission() ends the "write" phase and actually
    //    sends that register address to the sensor.
    //    If the sensor does not ACK, Wire.endTransmission() returns a nonzero error.
    //    We check that here. If endTransmission() != 0, we say "true" meaning "error."
    int errorCode = Wire.endTransmission(false);
    String resultString = "endTransmission() returned: " + String(errorCode);
    PrintText(LoggerDebugLevel_Debug, resultString);
    if (errorCode != 0) {
      PrintText(LoggerDebugLevel_Error, "I2C error occurred.");
    return true;
    }

    // 3) Now that the ADXL345's internal pointer is set to DATAX0,
    //    we can request 6 bytes (X0, X1, Y0, Y1, Z0, Z1) from the sensor in a "read" transaction.
    Wire.requestFrom(ADXL345_I2C_ADDRESS, 6);
    PrintText(LoggerDebugLevel_Debug, "ADXL345, request 6 byte");

    // 4) Check if we actually received 6 bytes in the I2C buffer.
    //    If not, something went wrong (device not responding, bus error, etc.).
    if (Wire.available() != 6) {
        PrintText(LoggerDebugLevel_Error, "ADXL345 not enought data");
        return true; // Error: Not enough data received
    }

    // 5) Read those 6 bytes in pairs: (X0|X1), (Y0|Y1), (Z0|Z1).
    //    We'll assemble them into three 16-bit signed values (X, Y, Z).
    for (int i = 0; i < ADXL345_DataLength; i++) {
        // The register layout is LSB first then MSB, so we read in that order.
        ADXLArray[i] = (Wire.read() | (Wire.read() << 8));
    }
    // 6) Optionally offset or convert them into unsigned 16-bit values.
    //    The code below adds (1 << (ADXL345_Range + 1)) to each axis, then stores in AccelerometerValues.
    //    It's presumably some offset to shift negative readings into a 0-based range.
    for (int i = 0; i < ADXL345_DataLength; i++) {
        AccelerometerValues[i] = (uint16_t)(ADXLArray[i] + (1 << (ADXL345_Range + 1)));
    }
    // 7) Update our global timestamp to know when we last got fresh data.
    AccelerometerTimestamp = millis();
    // Returning false means "no error" (i.e., success).
    return false;
}

void TestAccelerometerModule() {
    int counter = 0;
    // Setup accelerometer
    if (AccelerometerSetup()) {
        PrintText(LoggerDebugLevel_Error, "Failed to initialize accelerometer!");
        return;
    }
    PrintText(LoggerDebugLevel_Debug, "Accelerometer initialized successfully!");

    // Read accelerometer values
    while (counter <100) {
        if (AccelerometerRead()) {
            PrintText(LoggerDebugLevel_Error, "Error reading accelerometer values!");
        } else {
            String resultString = "X: " + String(AccelerometerValues[0]) + " Y:" + String(AccelerometerValues[1]) + " Z:" + String(AccelerometerValues[2]) + "Ctr: " + String(counter);
            PrintText(LoggerDebugLevel_Info, resultString);
        }
        counter++;
    }
}