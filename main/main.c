/*
 * @file    main.c
 * @author  Jeremy Urena
 * @board   ESP-S3-DevKitC-1 v1.1
 * This is an RTOS-based firmware for authenticating a pet at a door using a PN532 RFID Reader
 * 
 * Main purpose: Get the servo to rotate 90 degrees when card is read. 
 *               Rotate 90 degrees back once IR break beam is unbroken 
 *               and after timer runs out.
 */

#include <stdio.h>
#include <stdlib.h>

#include <esp_log.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

#include "iot_servo.h"
#include "pn532.h"
#include "pn532_driver_i2c.h"

#include "inc/door_control.h"
#include "inc/pn532_rfid.h"

/* For IR Beam:
 *      First enable IO MUX for GPIO 6 to be in input mode
 *      Then, read from 6th bit position from GPIO input register
 */
uint32_t volatile * const IR_IO_MUX_GPIO6_REG  = (uint32_t *) (0x60009000 + (0x0004 + 4 * 6));
uint32_t volatile * const IR_GPIO_IN_REG       = (uint32_t *) (0x60004000 + 0x003C);

static const uint32_t UID_VAL = 0x97F6B001;

// ESP Log Tags
static const char *TAG_PN532 = "ntag_read";
static const char *TAG_SERVO = "servo_control";
static const char *TAG_IR    = "break_beam";

// M996R Servo Calibration Values
static uint16_t servo_calibration_val_0 = 20;
static uint16_t servo_calibration_val_180 = 200;

// M996R Servo Config
servo_config_t servo_config = {
    .max_angle      = SERVO_MAX_ANGLE,
    .min_width_us   = SERVO_MIN_WIDTH,
    .max_width_us   = SERVO_MAX_WIDTH,
    .freq           = SERVO_FREQ,
    .timer_number   = SERVO_TIMER_NUM,
    .channels = {
        .servo_pin = {
            SERVO_PIN,
        },
        .ch = {
            SERVO_CHANNEL,
        }
    },
    .channel_number = SERVO_CHANNEL_NUM
};

void app_main() 
{
    printf("APP MAIN\n");
    
    /*
     * Start Init Section
     */
    
    ir_init(TAG_IR, IR_IO_MUX_GPIO6_REG);
    servo_init(TAG_SERVO, &servo_config, SERVO_SPEED_MODE);
    pn532_io_t pn532_io;
    esp_err_t err;

    ESP_LOGI(TAG_PN532, "INIT PN532 IN I2C MODE");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, I2C_PORT_NUM, &pn532_io));
    do 
    {  
        err = pn532_init(&pn532_io);
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG_PN532, "FAILED TO INIT PN532");
            pn532_release(&pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }      
    } while (err != ESP_OK);
    
    /*
     * End Init Section
     */
    
    ESP_LOGI(TAG_PN532, "WAITING FOR AN ISO14443A CARD...");

    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uid_length = 0;
    uint32_t uid_value = 0;

    // Main loop
    for (;;)
    {
        // Reset to 0 to avoid lingering values after next iteration
        memset(uid, 0, sizeof(uid));
        uid_length = 0;

        err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG_PN532, "\nFOUND ISO14443A CARD!");
            
            /*
            ESP_LOGI(TAG_PN532, "UID LENGTH: %d BYTES", uid_length);
            ESP_LOGI(TAG_PN532, "UID VALUE: ");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG_PN532, uid, uid_length, ESP_LOG_INFO);
            */
            
            uid_value = find_uid_value(uid, uid_length);

            if (uid_value == UID_VAL)
            {
                iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, (servo_calibration_val_180 / 2) + 10); // Slight offset for 90 degrees
                do
                {
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG_IR, "IR GPIO VALUE: %d", ((*IR_GPIO_IN_REG >> 6) & 0x1));
                } while (!((*IR_GPIO_IN_REG >> 6) & 0x1)); // Busy wait; first delay to enter the door and break beam
                
                vTaskDelay(2000 / portTICK_PERIOD_MS); // Short after-protection
                iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, servo_calibration_val_0);
            }
        }
    }
}