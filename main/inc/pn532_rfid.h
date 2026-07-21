/*
 * pn532_rfid.h
 *
 * Definitions PN532 RFID sensor using I2C
 */

#include <stdint.h>

#ifndef PN532_RFID_H
#define PN532_RFID_H

#define SCL_PIN    (8)
#define SDA_PIN    (9)
#define RESET_PIN  (-1) // Could be configured if valid
#define IRQ_PIN    (4)

uint32_t find_uid_value(uint8_t arr[], uint8_t length);

#endif