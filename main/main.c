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
#include <stdbool.h>

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

void task_main(void * parameters);

// Global Flag
volatile bool is_outside = false;

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

    xTaskCreate(task_main, "Main Task", 4096, NULL, 5, NULL);
}

void task_main(void * parameters)
{
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
}

/*
 * Things to do:
 *  Add global is_outside flag:         done
 *  Write RFID detection function:     
 *  Write IR beam detection function:   
 *  
 *  
 * 

Changing the logic:
 *
 * When cat is outside: RFID -> Unlock door -> Have motion be detected by IR sensor -> Lock door after cat is out of the way and timer runs out
 * 
 * Before cat walks up:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door locked
 * 
 * As cat approaches:
 *  RFID detection function RUNNING: detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: detected
 *  Outside_flag = false
 *  Door unlocked
 * 
 * Right after cat moves away from beam:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: not detected
 *  Timer starts = 5 secs
 *  Outside_flag = false
 *  Door unlocked
 * 
 * After timer runs out:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING; not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door locked
 * 
 * **If cat detected before timer runs out:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: not detected
 *  Timer resets = 5 secs
 *  Outside_flag = false
 *  Door unlocked
 * 
 * 
 * 
 * When cat is inside: IR beam broken with NO AUTHENTICATION (since it's not required when he's already inside the house) -> Lock door after cat is out of the way and timer runs out
 * 
 * Before cat walks up:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = false
 *  Door locked
 * 
 * As cat approaches:
 *  RFID detection function NOT RUNNING
 *  IR inside_detection function RUNNING: detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door unlocked
 * 
 * Right after cat moves away from beam
 *  RFID detection function NOT RUNNING
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Timer starts = 5 secs
 *  Door unlocked
 * 
 * After timers runs out
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door locked
 * 
 * **If cat detected before timer runs out:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: not detected
 *  Timer resets = 5 secs
 *  Outside_flag = true
 *  Door unlocked

*/