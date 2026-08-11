#include "inc/main.h"

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

void rfid_init(const char * tag, rfidParams_t * params, const uint32_t uid_val, SemaphoreHandle_t * mutex, uint32_t volatile * const ir_gpio_in_reg)
{   
    params->tag = tag;
    params->uid_val = uid_val;
    params->mutex = mutex;
    params->ir_gpio_in_reg = ir_gpio_in_reg;

    // I2C Device Init
    ESP_LOGI(params->tag, "INIT PN532 IN I2C MODE");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, I2C_PORT_NUM, &params->pn532_io));
    do 
    {  
        // PN532 Init
        params->err = pn532_init(&params->pn532_io);
        if (params->err != ESP_OK)
        {
            ESP_LOGW(params->tag, "FAILED TO INIT PN532");
            pn532_release(&params->pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }      
    } while (params->err != ESP_OK);
}

void task_rfid_detect(void * pvParameters)
{
    rfidParams_t * params = (rfidParams_t *) pvParameters;

    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uid_length = 0;
    uint32_t uid_value = 0;
    
    ESP_LOGI(params->tag, "WAITING FOR AN ISO14443A CARD...");
    
    // Main loop
    for (;;)
    {
        // Reset to 0 to avoid lingering values after next iteration
        memset(uid, 0, sizeof(uid));
        uid_length = 0;

        if (xSemaphoreTake(*params->mutex, portMAX_DELAY) == pdTRUE)
        {
            params->err = pn532_read_passive_target_id(&params->pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 100);
            
            if (params->err == ESP_OK)
            {
                ESP_LOGI(params->tag, "FOUND ISO14443A CARD!");

                uid_value = find_uid_value(uid, uid_length);
                if (uid_value == params->uid_val)
                {
                    ESP_LOGI(params->tag, "CORRECT UID");
                    
                    servo_open_close(params->ir_gpio_in_reg, SERVO_CALIBRATION_VAL_0, SERVO_CALIBRATION_VAL_180);
                }
                else 
                {
                    ESP_LOGI(params->tag, "INCORRECT UID");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
            }

            xSemaphoreGive(*params->mutex);
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}