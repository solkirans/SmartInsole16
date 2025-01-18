#ifndef BLUETOOTH_MODULE_H
#define BLUETOOTH_MODULE_H

#include <Arduino.h>

// ------------------------------
// Parameters
// ------------------------------
static const char* INSOLE_NAME_RIGHT = "Insole Right";
static const char* INSOLE_NAME_LEFT  = "Insole Left";

// Right-Side UUIDs
static const char* SERVICE_UUID_RIGHT        = "00000001-ABCD-1234-5678-0123456789AB";
static const char* CHARACTERISTIC_UUID_RIGHT = "00000002-ABCD-1234-5678-0123456789AB";

// Left-Side UUIDs
static const char* SERVICE_UUID_LEFT         = "00000003-ABCD-1234-5678-0123456789AB";
static const char* CHARACTERISTIC_UUID_LEFT  = "00000004-ABCD-1234-5678-0123456789AB";

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
 * @return true if successfully sent (or at least attempted), false if not connected
 */
bool BLE_SendByteArray(uint8_t* msg);

/**
 * @brief A unit test for the Bluetooth module. Initializes BLE
 *        (using "Insole Right" as an example) and sends a 39-byte test message.
 */
void BLE_Test(void);

#endif // BLUETOOTH_MODULE_H
