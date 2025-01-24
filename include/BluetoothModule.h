#ifndef BLUETOOTH_MODULE_H
#define BLUETOOTH_MODULE_H

#include <Arduino.h>


// /////////////////////////////////////////////////////////////////
// ''''''' BLE ''''''''''''''''''' //

// BLE delay after message sent
#define BLE_MSG_DELAY 1 // 1 ms




// BLE task stack size
#define BLE_TASK_STACK_SIZE   16384
// ------------------------------
// Advertising intervals
// ------------------------------
#define ADVERTISING_INTERVAL_MIN 0x50  // 0x50 * 0.625ms = ~31.25ms
#define ADVERTISING_INTERVAL_MAX 0x100 // 0x100 * 0.625ms = ~160ms

// Add configurable parameter for power:
#define BLE_TX_POWER ESP_PWR_LVL_P9

// ------------------------------
// Parameters
// ------------------------------
static const char* INSOLE_NAME_RIGHT = "Insole Right";
static const char* INSOLE_NAME_LEFT  = "Insole Left";

// Right-Side UUIDs
static const char* SERVICE_UUID_RIGHT        = "4276d6a5-3c2e-494a-bee2-0357f0c8e7f1";
static const char* CHARACTERISTIC_UUID_RIGHT = "af4ce09f-721e-459b-9ba5-b1a743073afa";

// Left-Side UUIDs
static const char* SERVICE_UUID_LEFT         = "e59f97e5-31c5-4d8c-bd07-27b9c0284d31";
static const char* CHARACTERISTIC_UUID_LEFT  = "10480c36-db9c-476a-8ecf-129aa85243b8";




// --------------------------------------------------------------
// Functions
// --------------------------------------------------------------

/**
 * @brief Initializes the BLE module for either left or right insole.
 * @param FlagSide: if 0 => left, if 1 => right.
 *        This selects which name & UUIDs to use.
 * @return true if success, false otherwise
 */
bool BLE_Init(bool FlagSide);

/**
 * @brief Sends a 39-byte message immediately via BLE (if connected).
 * @param msg A pointer to the 39-byte array to send
 * @return true if successfully sent, false if not connected
 */
bool BLE_SendBuffer(uint8_t* data);
/**
 * @brief A unit test for the Bluetooth module. Initializes BLE
 *        (using "Insole Right" as an example) and sends a 39-byte test message.
 */
void BLE_Test(void);

uint8_t BLE_GetNumOfSubscribers(void);
#endif // BLUETOOTH_MODULE_H
