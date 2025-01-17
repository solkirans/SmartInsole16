#include "PressureSensorModule.h"
#include "Logger.h"
// Define variables here (allocated once)
const uint8_t ADS1115_Address[ADS1115_Count] = { 0x48, 0x49, 0x4A, 0x4B };
uint16_t PressureSensorValues[ADS1115_TotalChannel_Count] = { 0 };
uint32_t PressureMeasurementTimeStamp = 0;
uint8_t ADS1115_Status[ADS1115_Count] = { ADS1115_Status_Ok, ADS1115_Status_Ok, ADS1115_Status_Ok, ADS1115_Status_Ok };

Adafruit_ADS1115 ads[ADS1115_Count];

bool ADCSetup() {
  Wire.begin(I2C_SDA_Pin, I2C_SCL_Pin);
  Wire.setClock(400000);  // Set I2C to 400kHz

  bool allOk = true;
  for (uint8_t i = 0; i < ADS1115_Count; i++) {
    if (ads[i].begin(ADS1115_Address[i])) {
      ads[i].setGain(ADS1115_Gain);  // Set gain to 4.096V range
      ads[i].setDataRate(ADS1115_SPS);
      ADS1115_Status[i] = ADS1115_Status_Ok;
    } else {
      ADS1115_Status[i] = ADS1115_Status_InitFail;
      PrintText(LoggerDebugLevel_Error, "ADS1115 Setup Return: ADS1115_Status_InitFail");
      allOk = false;
    }
  }
  return allOk;
}

bool ADCRead() {
  bool allOk = true;

  for (uint8_t i = 0; i < ADS1115_Count; i++) {
    if (ADS1115_Status[i] != ADS1115_Status_Ok) {
      continue;  // Skip ICs that failed initialization
    }

    for (uint8_t channel = 0; channel < ADS1115_Channel_Count; channel++) {
      //ads[i].startADCReading(channel, false);             // Initiate ADC read on the channel
      //delay(2);                                           // Allow conversion to complete (depends on data rate)
      int16_t value = ads[i].readADC_SingleEnded(channel);  // Get the ADC result
      if (value == -1) {                                  // Check for read error
        ADS1115_Status[i] = ADS1115_Status_ReadFail;
        PrintText(LoggerDebugLevel_Error, "ADS1115 Read Return: ADS1115_Status_ReadFail");
        allOk = false;
      } else {
        PressureSensorValues[i * ADS1115_Channel_Count + channel] = value;
      }
    }
  }

  PressureMeasurementTimeStamp = millis();
  return allOk;
}

void testPressureSensorModule() {
  if (!ADCSetup()) {
    PrintText(LoggerDebugLevel_Error, "ADC Setup failed");
    return;
  }

  for (int i = 0; i < 100; i++) {
    if (!ADCRead()) {
      PrintText(LoggerDebugLevel_Error, "ADC Read failed");
    } else {
      String msg = "Val:";
      for (int j=0; j<ADS1115_TotalChannel_Count; j++)
      {
        msg += String(PressureSensorValues[j])+","; 
      }
      PrintText(LoggerDebugLevel_Debug, msg);
    }
  }

  PrintText(LoggerDebugLevel_Debug, "Test completed successfully!");
}
