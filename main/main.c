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

void task_rfid_detect(void * pvParameters);
void task_ir_detect(void * pvParameters);
void ir_wait_for_cat(void);
void rfid_init(const char * tag, void * pvParameters);

// Global Flag
volatile bool is_outside = false;

/* For IR Beam:
 *      First enable IO MUX for GPIO 6 to be in input mode
 *      Then, read from 6th bit position from GPIO input register
 */
uint32_t volatile * const IR_IO_MUX_GPIO6_REG  = (uint32_t *) (0x60009000 + (0x0004 + 4 * 6));
uint32_t volatile * const IR_GPIO_IN_REG       = (uint32_t *) (0x60004000 + 0x003C);

// Physical Tag UIDs
static const uint32_t UID_VAL = 0x97F6B001;

// ESP Log Tags
static const char *TAG_PN532 = "ntag_read";
static const char *TAG_SERVO = "servo_control";
static const char *TAG_IR    = "break_beam";
// static const char *TAG_MAIN    = "main";


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

// Param Structs
typedef struct 
{
    esp_err_t err;
    pn532_io_t pn532_io;
} rfidParams_t;

// Param Inits
rfidParams_t rfid_params = {};

// Handlers
TaskHandle_t task_rfid_detect_hdl;
TaskHandle_t task_ir_detect_hdl;

// IR Detection Start Time
#define IR_START_TIME 5000

void app_main() 
{
    printf("APP MAIN\n");

    /*
    * Start Init Section
    */
    
    ir_init(TAG_IR, IR_IO_MUX_GPIO6_REG);
    servo_init(TAG_SERVO, &servo_config, SERVO_SPEED_MODE);
    rfid_init(TAG_PN532, &rfid_params);

    /*
    * End Init Section
    */

    xTaskCreate(task_rfid_detect, "RFID Outside Detection Task", 4096, &rfid_params, 5, &task_rfid_detect_hdl);
    xTaskCreate(task_ir_detect, "IR Inside Detection Task", 4096, NULL, 5, &task_ir_detect_hdl);
}

void task_rfid_detect(void * pvParameters)
{
    rfidParams_t * params = (rfidParams_t *) pvParameters;

    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uid_length = 0;
    uint32_t uid_value = 0;
    
    ESP_LOGI(TAG_PN532, "WAITING FOR AN ISO14443A CARD...");
    
    // Main loop
    for (;;)
    {
        // Reset to 0 to avoid lingering values after next iteration
        memset(uid, 0, sizeof(uid));
        uid_length = 0;

        params->err = pn532_read_passive_target_id(&params->pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);

        if (params->err == ESP_OK)
        {
            ESP_LOGI(TAG_PN532, "FOUND ISO14443A CARD!");

            // SUSPEND IR DETECTION FUNCTION HERE !!!
            vTaskSuspend(task_ir_detect_hdl);

            uid_value = find_uid_value(uid, uid_length);
            if (uid_value == UID_VAL)
            {
                ESP_LOGI(TAG_PN532, "CORRECT UID");

                is_outside = false;
                
                // Slight offset for 90 degrees
                iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, servo_calibration_val_0);
                ir_wait_for_cat();
                iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, (servo_calibration_val_180 / 3) + 10);
            }
            else 
            {
                ESP_LOGI(TAG_PN532, "INCORRECT UID");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

            // RESUME IR DETECTION FUNCTION HERE !!!
            vTaskResume(task_ir_detect_hdl);
        }
    }
}

void task_ir_detect(void * pvParameters)
{
    for(;;)
    {
        if (!((*IR_GPIO_IN_REG >> 6) & 0x1))
        {
            // SUSPEND RFID DETECTION FUNCTION HERE !!!
            vTaskSuspend(task_rfid_detect_hdl);
            
            // Slight offset for 90 degrees
            iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, servo_calibration_val_0);
            ir_wait_for_cat();
            iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, (servo_calibration_val_180 / 3) + 10);

            // RESUME RFID DETECTION FUNCTION HERE !!!
            vTaskResume(task_rfid_detect_hdl);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void rfid_init(const char * tag, void * pvParameters)
{
    rfidParams_t * params = (rfidParams_t *) pvParameters;
    
    // I2C Device Init
    ESP_LOGI(TAG_PN532, "INIT PN532 IN I2C MODE");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, I2C_PORT_NUM, &params->pn532_io));
    do 
    {  
        // PN532 Init
        params->err = pn532_init(&params->pn532_io);
        if (params->err != ESP_OK)
        {
            ESP_LOGW(TAG_PN532, "FAILED TO INIT PN532");
            pn532_release(&params->pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }      
    } while (params->err != ESP_OK);
}

void ir_wait_for_cat(void)
{
    uint16_t timer = IR_START_TIME; // in milliseconds

    while (timer > 0)
    {
        vTaskDelay(250 / portTICK_PERIOD_MS);
        timer -= 250; // 
        if (!((*IR_GPIO_IN_REG >> 6) & 0x1))
        {
            timer = IR_START_TIME; // Reset timer
        }
    }
}

/*
 * Things to do:
 *  Add global is_outside flag:         done
 *  Write RFID detection function:      
 *  Write IR detection function:        
 *  Write IR wait for cat function:     done
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