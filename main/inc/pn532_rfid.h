/*
 * @file        pn532_rfid.h
 * Definitions for PN532 RFID sensor-related functions
 */

#ifndef PN532_RFID_H
#define PN532_RFID_H

#include "inc/main.h"

typedef struct 
{
    const char * tag;
    uint32_t volatile * ir_gpio_in_reg;
    uint32_t uid_val;
    esp_err_t err;
    pn532_io_t pn532_io;
    SemaphoreHandle_t * mutex;
} rfidParams_t;

#define SCL_PIN         (8)
#define SDA_PIN         (9)
#define RESET_PIN       (-1) // Could be configured if valid
#define IRQ_PIN         (4)
#define I2C_PORT_NUM    (0)

/**
 * @brief Initalize PN532 sensor for task_rfid_detect task
 * 
 * @param tag RFID tag for debugging
 * @param params Param struct
 * @param uid_val Compare value for RFID tag
 * @param ir_gpio_in_reg Input register to read IR bit
 * @param mutex Used by both task_rfid_detect and task_ir_detect
 * 
 */
void task_rfid_init(const char * tag, rfidParams_t * params, const uint32_t uid_val, uint32_t volatile * const ir_gpio_in_reg, SemaphoreHandle_t * mutex);

/**
 * @brief Searches for ISO14443A ID card/tag and compares with allowlist. If in allowlist, it unlocks the door and waits until pet leaves.
 * 
 * @param pvParameters Params found in rfidParams_t struct
 * 
 */
void task_rfid_detect(void * pvParameters);

/**
 * @brief Used for converting UID buffer into long int for comparison
 * 
 * @param uid UID array buffer from pn532_read_passive_target_id
 * @param uid_length UID length returned from pn532_read_passive_target_id
 */
uint64_t find_uid_value(uint8_t uid[], uint8_t uid_len);

#endif