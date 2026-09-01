/*
 * @file    main.c
 * @author  Jeremy Urena
 * @board   ESP-S3-DevKitC-1 v1.1
 * This is an RTOS-based firmware for authenticating a pet at a door using a PN532 RFID Reader
 * 
 */

#include "inc/main.h"

/* Physical Tag UIDs */
static const uint32_t UID_VAL = 0x97F6B001;

/* ESP Log Tags */
static const char *TAG_PN532 = "PN532";
static const char *TAG_SERVO = "SERVO";
static const char *TAG_IR    = "IR";
static const char *TAG_MAIN  = "MAIN";

/* M996R Servo Config */
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

/* Param Inits */
rfidParams_t rfid_params = {};
irParams_t ir_params = {};

/* Handlers */
TaskHandle_t hdl_task_rfid_detect;
TaskHandle_t hdl_task_ir_detect;

/* Mutex */
SemaphoreHandle_t rfid_hw_mutex;

void app_main() 
{
    /* Start Init Section */
    
    task_rfid_init(TAG_PN532, &rfid_params, UID_VAL, IR_GPIO_IN_REG, &rfid_hw_mutex);
    task_ir_init(TAG_IR, &ir_params, IR_IO_MUX_GPIO6_REG, IR_GPIO_IN_REG, IR_GPIO_NUM, &rfid_hw_mutex);
    servo_init(TAG_SERVO, &servo_config, SERVO_SPEED_MODE);

    /* End Init Section */


    /* Start FreeRTOS-related Section */

    rfid_hw_mutex = xSemaphoreCreateMutex();
    if (rfid_hw_mutex == NULL)
    {
        ESP_LOGE(TAG_MAIN, "FAILED TO CREATE MUTEX - RESTARTING");
        esp_restart();
    }

    xTaskCreate(task_rfid_detect, "RFID Outside Detection Task", 2048, &rfid_params, 5, &hdl_task_rfid_detect);
    xTaskCreate(task_ir_detect, "IR Inside Detection Task", 2048, &ir_params, 5, &hdl_task_ir_detect);

    /* End FreeRTOS-related Section */
}