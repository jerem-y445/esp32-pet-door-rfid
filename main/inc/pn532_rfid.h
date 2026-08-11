/*
 * pn532_rfid.h
 *
 * Definitions PN532 RFID sensor using I2C
 */

#include "inc/main.h"

#ifndef PN532_RFID_H
#define PN532_RFID_H

typedef struct 
{
    const char * tag;
    esp_err_t err;
    pn532_io_t pn532_io;
    uint32_t uid_val;
    SemaphoreHandle_t * mutex;
    uint32_t volatile * ir_gpio_in_reg;
} rfidParams_t;

#define SCL_PIN         (8)
#define SDA_PIN         (9)
#define RESET_PIN       (-1) // Could be configured if valid
#define IRQ_PIN         (4)
#define I2C_PORT_NUM    (0)


uint32_t find_uid_value(uint8_t arr[], uint8_t length);


void rfid_init(const char * tag, rfidParams_t * params, const uint32_t uid_val, SemaphoreHandle_t * mutex, uint32_t volatile * const ir_gpio_in_reg);


void task_rfid_detect(void * pvParameters);


#endif