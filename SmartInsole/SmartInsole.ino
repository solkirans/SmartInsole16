#include <Arduino.h>
#include "Logger.h"
#include "PressureSensorModule.h"
#include "AccelerometerModule.h"
#include <stdint.h>
// Setup function: runs once when the program starts
void setup() {
  Serial.begin(115200);
  PrintText(LoggerDebugLevel_Debug,"Setup");
  // Initialize Logger
  SetupLogger();
  PrintText(LoggerDebugLevel_Debug,"SetupLogger Done.");
  // Initialize Pressure Sensor ADC
  ADCSetup();
  PrintText(LoggerDebugLevel_Debug,"ADCSetup Done.");
  // Initialize Accelerometer Module
  AccelerometerSetup();
  PrintText(LoggerDebugLevel_Debug,"AccelerometerSetup Done.");
  //testFunctions();
}

void testFunctions()
{
  PrintText(LoggerDebugLevel_Debug,"Starting Tests....");

  PrintText(LoggerDebugLevel_Debug, "Starting testPressureSensorModule......");
  testPressureSensorModule();

  PrintText(LoggerDebugLevel_Debug, "Starting TestAccelerometerModule......");
  TestAccelerometerModule();

}

void loop() {
    
  // Read ADC values
  if (!ADCRead()) {
    PrintText(LoggerDebugLevel_Error, "ADC read failed!");
    return;  // Skip the rest of the loop if ADC read fails
  }

  // Process Pressure Sensor Values
  String PressureSensorValuesString = "";
  for (int i = 0; i < ADS1115_TotalChannel_Count; i++) {
    PressureSensorValuesString += String(PressureSensorValues[i]) + ",";
  }

  // Send and log Pressure Sensor Values
  sendPressureArray(PressureSensorValues, ADS1115_TotalChannel_Count, PressureMeasurementTimeStamp);
  PrintText(LoggerDebugLevel_Info, "Pressure Sensor Values: " + PressureSensorValuesString);


  // Read Accelerometer values
  if (AccelerometerRead()) {
    PrintText(LoggerDebugLevel_Error, "Accelerometer read failed!");
    //return;  // Skip the rest of the loop if Accelerometer read fails
  }
  // Process Accelerometer Sensor Values
  String AccelerometerSensorValuesString = "";
  for (int i = 0; i < ADXL345_DataLength; i++) {
    AccelerometerSensorValuesString += String(AccelerometerValues[i]) + ",";
  }

  // Send and log Accelerometer Sensor Values
  sendAccelerometerArray(AccelerometerValues, ADXL345_DataLength, AccelerometerTimestamp);
  PrintText(LoggerDebugLevel_Info, "Accelerometer Sensor Values: " + AccelerometerSensorValuesString);







  
}
