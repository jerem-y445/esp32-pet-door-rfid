#include "inc/main.h"

void task_rfid_init(const char * tag, rfidParams_t * params, const uint32_t uid_val, uint32_t volatile * const ir_gpio_in_reg, SemaphoreHandle_t * mutex)
{   
    ESP_LOGI(tag, "INIT START");
    
    params->tag = tag;
    params->uid_val = uid_val;
    params->mutex = mutex;
    params->ir_gpio_in_reg = ir_gpio_in_reg;

    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, I2C_PORT_NUM, &params->pn532_io));
    
    do 
    {
        params->err = pn532_init(&params->pn532_io);
        if (params->err != ESP_OK)
        {
            ESP_LOGW(params->tag, "FAILED TO INIT PN532");
            pn532_release(&params->pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }      
    } while (params->err != ESP_OK);
    
    ESP_LOGI(params->tag, "INIT SUCCESSFUL");
}

void task_rfid_detect(void * pvParameters)
{
    rfidParams_t * params = (rfidParams_t *) pvParameters;

    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uid_length = 0;
    uint64_t uid_value = 0;
    
    ESP_LOGI(params->tag, "WAITING FOR AN ISO14443A TAG...");

    for (;;)
    {   
        // Reset to 0 to avoid lingering values after next iteration
        memset(uid, 0, sizeof(uid));
        uid_length = 0;

        params->err = pn532_read_passive_target_id(&params->pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);
        if (xSemaphoreTake(*params->mutex, 0) == pdTRUE) 
        { 
            if (params->err == ESP_OK)
            {
                ESP_LOGI(params->tag, "ISO14443A TAG FOUND");

                if (uid_length >= 1)
                {
                    uid_value = find_uid_value(uid, uid_length);
                }
                else
                {
                    ESP_LOGW(params->tag, "\"uid_length\" IS LESS THAN 1; INVALID");
                    continue;
                }

                if (uid_value == params->uid_val)
                {
                    ESP_LOGI(params->tag, "CORRECT UID");
                    servo_open_close(params->ir_gpio_in_reg, SERVO_CALIBRATION_VAL_0, SERVO_CALIBRATION_VAL_180);
                }
                else { ESP_LOGI(params->tag, "INCORRECT UID"); }
            }
            xSemaphoreGive(*params->mutex);
        }

        vTaskDelay(200);
    }
}

uint64_t find_uid_value(uint8_t uid[], uint8_t uid_len) 
{
    uint64_t concat_val = uid[0];

    for (uint8_t i = 0; i < uid_len - 1; ++i)
    {
        concat_val = (concat_val << 8);
        concat_val += uid[i + 1];
    }

    return concat_val;
}