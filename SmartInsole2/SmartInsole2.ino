#include <Adafruit_ADS1X15.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_MAX1704X.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "UUID.h"
#include <string>

// i2c ayarları
#define SDA 21 
#define SCL 22
#define frq 800000

// i2c adres ayarları
#define ADS1X15_ADDRESS1 0x48
#define ADS1X15_ADDRESS2 0x49
#define ADS1X15_ADDRESS3 0x4a
#define ADS1X15_ADDRESS4 0x4b
#define ADXL345_ADDRESS 0x53
#define MAX17048_ADDRESS 0x36

// BluetoothSerial SerialBT;

Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;
Adafruit_ADS1115 ads3;
Adafruit_ADS1115 ads4;

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

Adafruit_MAX17048 maxlipo;

UUID uuid;

BLEServer *pServer;
BLECharacteristic *pCharacteristic;
BLEService *pService;
BLEAdvertising *pAdvertising;

std::string btname = "ESP32";
uint16_t adc[16];
uint32_t sayac = 0;
float volts[16], x, y, z, voltage, percentage, gain = 1.0, temperature = NAN;
double adxlmultiplier;
String data = "";
bool success = false;
static const char *const TAG = "SmartInsole";

void internal_temp();
void uuid_set();
void bt_set();
void bt();
void i2c_set();
void ads1115_set();
void ads1115();
void adxl345_set();
void adxl345();
void max17048_set();
void max17048();

void setup();
void dump_config();
void loop();
void update();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  UUID
void uuid_set()
{
    uint32_t seed1 = random(999999999);
    uint32_t seed2 = random(999999999);
    uuid.seed(seed1, seed2);
    uuid.generate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Bluetooth
void bt_set()
{
    BLEDevice::init(btname);  

    pServer = BLEDevice::createServer();

    pService = pServer->createService(uuid.toCharArray());

    pCharacteristic = pService->createCharacteristic(
                                                    uuid.toCharArray(),
                                                    BLECharacteristic::PROPERTY_READ |
                                                    BLECharacteristic::PROPERTY_WRITE |
                                                    BLECharacteristic::PROPERTY_NOTIFY
                                                    );

    // pCharacteristic->setValue("Hello World");
    pService->start();

    pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    // // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    // pAdvertising->start();
    pAdvertising->addServiceUUID(uuid.toCharArray());
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    // pAdvertising->setMinPreferred(0x12);
    // // pAdvertising->setScanResponse(false);
    // pAdvertising->setMinPreferred(0x00);
    BLEDevice::startAdvertising();

}
void bt()
{
    // ESP_LOGI(TAG, "data: %s", data);
    
    pCharacteristic->setValue(data.c_str());
    Serial.println(data.c_str());
    pCharacteristic->notify();

    data = "";

    sayac += 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  i2c
void i2c_set()
{
    Wire.begin(SDA,SCL,frq);
    //Wire1.begin(SDA_1, SCL_1, freq_1);
    //I2C_1.begin(SDA_1, SCL_1, freq_1);
    //I2C_2.begin(SDA_2, SCL_2, freq_2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ADS1115
void ads1115_set()
{  
    //bool status1 = ads.begin(address1,&I2C);
    //bool status2 = ads2.begin(address2);
    //bool status3 = ads3.begin(address3);
    //bool status4 = ads4.begin(address4);

    if (!ads1.begin(ADS1X15_ADDRESS1, &Wire))
    {
        ESP_LOGE(TAG, "Failed to initialize ADS1115_1.");
        while (1);
    }
    if (!ads2.begin(ADS1X15_ADDRESS2, &Wire))
    {
        ESP_LOGE(TAG, "Failed to initialize ADS1115_2.");
        while (1);
    }
    if (!ads3.begin(ADS1X15_ADDRESS3, &Wire))
    {
        ESP_LOGE(TAG, "Failed to initialize ADS1115_3.");
        while (1);
    }
    if (!ads4.begin(ADS1X15_ADDRESS4, &Wire))
    {
        ESP_LOGE(TAG, "Failed to initialize ADS1115_4.");
        while (1);
    }


    ads1.setGain(GAIN_ONE); 
    ads2.setGain(GAIN_ONE); 
    ads3.setGain(GAIN_ONE);
    ads4.setGain(GAIN_ONE);

    ads1.setDataRate(RATE_ADS1115_860SPS);
    ads2.setDataRate(RATE_ADS1115_860SPS);
    ads3.setDataRate(RATE_ADS1115_860SPS);
    ads4.setDataRate(RATE_ADS1115_860SPS);

}
void ads1115()
{
    for(int i=0;i<4;i++)
    {
        adc[i] = ads1.readADC_SingleEnded(i%4);
        volts[i] = ads1.computeVolts(adc[i]) * gain;
        data += String(volts[i]) + ",";
    }
    for(int i=4;i<8;i++)
    {
        adc[i] = ads2.readADC_SingleEnded(i%4);
        volts[i] = ads2.computeVolts(adc[i]) * gain;
        data += String(volts[i]) + ",";
    }
    for(int i=8;i<12;i++)
    {
        adc[i] = ads3.readADC_SingleEnded(i%4);
        volts[i] = ads3.computeVolts(adc[i]) * gain;
        data += String(volts[i]) + ",";
    }
    for(int i=12;i<16;i++)
    {
        adc[i] = ads4.readADC_SingleEnded(i%4);
        volts[i] = ads4.computeVolts(adc[i]) * gain;
        data += String(volts[i]) + ",";
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ADXL345
void adxl345_set()
{
    if (!accel.begin(ADXL345_ADDRESS))
    {
        ESP_LOGE(TAG, "Failed to initialize ADXL345.");
        while (1);
    }

    /* Set the range to whatever is appropriate for your project */
    //  ADXL345_RANGE_16_G ///< +/- 16g
    //  ADXL345_RANGE_8_G  ///< +/- 8g
    //  ADXL345_RANGE_4_G  ///< +/- 4g
    //  ADXL345_RANGE_2_G  ///< +/- 2g (default value)
    accel.setRange(ADXL345_RANGE_2_G);
    
    //  ADXL345_DATARATE_3200_HZ  ///< 1600Hz Bandwidth   140�A IDD
    //  ADXL345_DATARATE_1600_HZ  ///<  800Hz Bandwidth    90�A IDD
    //  ADXL345_DATARATE_800_HZ  ///<  400Hz Bandwidth   140�A IDD
    //  ADXL345_DATARATE_400_HZ  ///<  200Hz Bandwidth   140�A IDD
    //  ADXL345_DATARATE_200_HZ  ///<  100Hz Bandwidth   140�A IDD
    //  ADXL345_DATARATE_100_HZ  ///<   50Hz Bandwidth   140�A IDD
    //  ADXL345_DATARATE_50_HZ   ///<   25Hz Bandwidth    90�A IDD
    //  ADXL345_DATARATE_25_HZ   ///< 12.5Hz Bandwidth    60�A IDD
    //  ADXL345_DATARATE_12_5_HZ ///< 6.25Hz Bandwidth    50�A IDD
    //  ADXL345_DATARATE_6_25HZ  ///< 3.13Hz Bandwidth    45�A IDD
    //  ADXL345_DATARATE_3_13_HZ ///< 1.56Hz Bandwidth    40�A IDD
    //  ADXL345_DATARATE_1_56_HZ ///< 0.78Hz Bandwidth    34�A IDD
    //  ADXL345_DATARATE_0_78_HZ ///< 0.39Hz Bandwidth    23�A IDD
    //  ADXL345_DATARATE_0_39_HZ ///< 0.20Hz Bandwidth    23�A IDD
    //  ADXL345_DATARATE_0_20_HZ ///< 0.10Hz Bandwidth    23�A IDD
    //  ADXL345_DATARATE_0_10_HZ ///< 0.05Hz Bandwidth    23�A IDD (default value)
    accel.setDataRate(ADXL345_DATARATE_100_HZ);
}
void adxl345()
{
    adxlmultiplier = ADXL345_MG2G_MULTIPLIER * SENSORS_GRAVITY_STANDARD;
    x = accel.getX() * adxlmultiplier;
    y = accel.getY() * adxlmultiplier;
    z = accel.getZ() * adxlmultiplier;
    data += String(x) + "," + String(y) + "," + String(z) + ",";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  MAX17048
void max17048_set()
{
    if (!maxlipo.begin(&Wire))
    {
        ESP_LOGE(TAG, "Failed to initialize MAX17048.");
        while (1);
    }
}
void max17048()
{
    voltage = maxlipo.cellVoltage();
    percentage = maxlipo.cellPercent();
    data += String(voltage) + "," + String(percentage) + ",";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Internal Temp
void internal_temp()
{
    data += String(temperatureRead());
}

void setup()
{ 
    Serial.begin(115200);
    uuid_set();
    bt_set();
    i2c_set();
    ads1115_set();
    adxl345_set();
    max17048_set();
}    
void dump_config()
{
    ESP_LOGI(TAG, "UUID: %s", uuid.toCharArray());
    ESP_LOGI(TAG, "Bluetooth Device name ready to pair: %s", btname);
}
void loop()
{
    ads1115();
    //adxl345();
    //max17048();
    //internal_temp();
    //bt();
    Serial.println(data.c_str());
    data = "";
    delay(100);
}