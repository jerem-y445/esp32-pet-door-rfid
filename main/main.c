/*
 * @file    main.c
 * @author  Jeremy Urena
 * @board   ESP-S3-DevKitC-1 v1.1
 * This is an RTOS-based firmware for authenticating a pet at a pet door using a PN532 RFID Reader
 */

#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "pn532_driver_i2c.h"
#include "pn532.h"
#include "iot_servo.h"

// For testing purposes
#define USE_VTASKS 0

#if USE_VTASKS

#define SCL_PIN    (8)
#define SDA_PIN    (9)
#define RESET_PIN  (-1) // Could be configured if valid
#define IRQ_PIN    (4)
#define SERVO_PIN  (5)

static const uint32_t UID_VAL = 0x97F6B001;

static const char *TAG_0 = "NTAG_READ";
static const char *TAG_1 = "SERVO_CTRL";

static uint16_t calibration_value_0 = 30;
static uint16_t calibration_value_180 = 195;

// Function prototypes
void vReaderRFIDTask(void *pvParameters);
void vServoTestTask(void *pvParameters);
void vCreateRFIDTask(void);
void vCreateServoTestTask(void);
uint32_t concatenateArray(uint8_t array[], uint8_t length);


// Error handling
typedef struct {
    pn532_io_t pn532_io;
    esp_err_t error;
} RFIDParams_t;

// RFID Init Params
static RFIDParams_t rfid_task_params = {
    .pn532_io = { 0 },
    .error = 0
};


void app_main() {
    ESP_LOGI(TAG_0, "APP MAIN");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    vCreateRFIDTask();

    vCreateServoTestTask();
}

void vServoTestTask(void *pvParameters) {
    ESP_LOGI(TAG_1, "SERVO TEST TASK");
    
    for ( ;; ) {
        for (int i = calibration_value_0; i <= calibration_value_180; ++i) {
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, i);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, calibration_value_0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void vReaderRFIDTask(void *pvParameters) {
    RFIDParams_t *config = (RFIDParams_t *)pvParameters;
    
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer to store the returned UID
    uint8_t uid_length = 0;                 // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    for ( ;; ) {
        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uid_length will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        config->error = pn532_read_passive_target_id(&config->pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);

        if (ESP_OK == config->error)
        {
            // Display some basic information about the card
            ESP_LOGI(TAG_0, "\nFOUND ISO14443A CARD");
            ESP_LOGI(TAG_0, "UID LENGTH: %d BYTES", uid_length);
            ESP_LOGI(TAG_0, "UID VALUE:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG_0, uid, uid_length, ESP_LOG_INFO);
        }

        if (concatenateArray(uid, uid_length) == UID_VAL) {
            ESP_LOGI(TAG_0, "WELCOME!");
        } else {
            ESP_LOGI(TAG_0, "WHO ARE YOU?");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Clear buffer for security
        memset(uid, 0, sizeof uid);
    }
}


void vCreateServoTestTask(void) {
    ESP_LOGI(TAG_1, "INIT SERVO CONTROL");

    servo_config_t servo_cfg = {
        .max_angle = 180,
        .min_width_us = 1000,
        .max_width_us = 2000,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = { SERVO_PIN },
            .ch        = { LEDC_CHANNEL_0 }
        },
        .channel_number = 1
    };

    iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);

    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    xReturned = xTaskCreate(vServoTestTask, "servoTest", 2048, NULL, 5, &xHandle);

    if (xReturned != pdPASS) {
        ESP_LOGI(TAG_0, "FAILED TO CREATE TASK!\n");
    }

}


void vCreateRFIDTask(void) {
    ESP_LOGI(TAG_0, "INIT PN532 IN I2C MODE");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, 0, &rfid_task_params.pn532_io));

    do {
        rfid_task_params.error = pn532_init(&rfid_task_params.pn532_io);
        if ( rfid_task_params.error != ESP_OK ) {
            ESP_LOGW(TAG_0, "FAILED TO INIT PN532");
            pn532_release(&rfid_task_params.pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while( rfid_task_params.error != ESP_OK );

    ESP_LOGI(TAG_0, "GET FIRMWARE VERSION");
    uint32_t version_data = 0;
    do {
        rfid_task_params.error = pn532_get_firmware_version(&rfid_task_params.pn532_io, &version_data);
        if (rfid_task_params.error != ESP_OK) {
            ESP_LOGI(TAG_0, "DID NOT FIND PN53x BOARD");
            pn532_reset(&rfid_task_params.pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while (rfid_task_params.error != ESP_OK);

    ESP_LOGI(TAG_0, "FOUND CHIP PN5%x", (unsigned int)(version_data >> 24) & 0xFF);
    ESP_LOGI(TAG_0, "FIRMWARE VERSION %d.%d", (int)(version_data >> 16) & 0xFF, (int)(version_data >> 8) & 0xFF);
    ESP_LOGI(TAG_0, "WAITING FOR ISO14443A CARD...");
    
    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    xReturned = xTaskCreate(vReaderRFIDTask, "rfidReader", configMINIMAL_STACK_SIZE * 3, ( void *) &rfid_task_params, tskIDLE_PRIORITY, &xHandle);
    
    if (xReturned != pdPASS) {
        ESP_LOGI(TAG_0, "FAILED TO CREATE TASK!\n");
    }
}

// Used for concatenating array-ed UID pulled from pn532_read_passive_target_id()
// for later comparison with stored UID
uint32_t concatenateArray(uint8_t array[], uint8_t length) {
    uint32_t concatValue = array[0];

    for (int i = 0; i < length - 1; ++i) {
        concatValue = (concatValue << 8);
        concatValue += array[i + 1];
    }

    return concatValue;
    
    /*
    unsigned pow;
    for (int i = 0; i < length; ++i) {
        pow = 10;
        while (array[i + 1] >= pow) {
            pow *= 10;
        }
    } 
    */   
    
}

#else

/*
 * Main purpose: Get the servo to rotate 90 degrees when card is read. 
 *               Rotate 90 degrees back after timer runs out.
 */

#define SCL_PIN    (8)
#define SDA_PIN    (9)
#define RESET_PIN  (-1) // Could be configured if valid
#define IRQ_PIN    (4)
#define SERVO_PIN  (5)

static const uint32_t UID_VAL = 0x97F6B001;
static const char *TAG_PN532 = "ntag_read";
static const char *TAG_SERVO = "servo_control";

static uint16_t servo_calibration_val_0 = 20;
static uint16_t servo_calibration_val_180 = 200;

uint32_t find_uid_value(uint8_t arr[], uint8_t length);
void servo_init(void);

void app_main() 
{
    // Create PN532 object
    pn532_io_t pn532_io;
    esp_err_t err;

    servo_init();

    printf("APP MAIN\n");

    // Only runs on I2C at the moment
    ESP_LOGI(TAG_PN532, "INIT PN532 IN I2C MODE");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, 0, &pn532_io));

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

    ESP_LOGI(TAG_PN532, "WAITING FOR AN ISO14443A CARD...");

    uint8_t uid[] = {0, 0, 0 , 0, 0 , 0, 0};
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
            ESP_LOGI(TAG_PN532, "UID LENGTH: %d BYTES", uid_length);
            ESP_LOGI(TAG_PN532, "UID VALUE: ");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG_PN532, uid, uid_length, ESP_LOG_INFO);
            
            uid_value = find_uid_value(uid, uid_length);
            ESP_LOGI(TAG_PN532, "UID VALUE AS INTEGER: %X", uid_value);

            if (uid_value == UID_VAL)
            {
                iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, (servo_calibration_val_180 / 2) + 10); // Slight offset for 90 degrees
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, servo_calibration_val_0);
            }

        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

uint32_t find_uid_value(uint8_t arr[], uint8_t length) 
{
    uint32_t concatValue = arr[0];

    for (int i = 0; i < length - 1; ++i) 
    {
        concatValue = (concatValue << 8);
        concatValue += arr[i + 1];
    }

    return concatValue;
}

void servo_init(void) 
{
    esp_err_t err;

    ESP_LOGI(TAG_SERVO, "INIT SERVO CONTROL");

    servo_config_t servo_config = {
        .max_angle = 180,
        .min_width_us = 500,
        .max_width_us = 2400,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = {
                SERVO_PIN,
            },
            .ch = {
                LEDC_CHANNEL_0,
            }
        },
        .channel_number = 1
    };

    err = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_config);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_SERVO, "FAILED TO INIT SERVO");
    }
}

#endif