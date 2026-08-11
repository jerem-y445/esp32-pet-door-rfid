/*
 * @file    main.c
 * @author  Jeremy Urena
 * @board   ESP32-S3-DevKitC-1 v1.1
 * This is an RTOS-based firmware for authenticating a pet at a door using a PN532 RFID Reader
 * 
 */

#include "inc/main.h"

/* Physical Tag UIDs */
static const uint32_t UID_VAL = 0x97F6B001;

/* ESP Log Tags */
static const char *TAG_PN532 = "ntag_read";
static const char *TAG_SERVO = "servo_control";
static const char *TAG_IR    = "break_beam";

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
TaskHandle_t task_rfid_detect_hdl;
TaskHandle_t task_ir_detect_hdl;

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

    xTaskCreate(task_rfid_detect, "RFID Outside Detection Task", 4096, &rfid_params, 5, &task_rfid_detect_hdl);
    xTaskCreate(task_ir_detect, "IR Inside Detection Task", 4096, &ir_params, 5, &task_ir_detect_hdl);

    /* End FreeRTOS-related Section */
}