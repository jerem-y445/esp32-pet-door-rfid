#include "inc/main.h"

/* IR Register Initalizations */
uint32_t volatile * const IR_IO_MUX_GPIO6_REG  = (uint32_t *) (0x60009000 + (0x0004 + 4 * 6));
uint32_t volatile * const IR_GPIO_IN_REG       = (uint32_t *) (0x60004000 + 0x003C);    

void task_ir_init(const char *tag, irParams_t * params, uint32_t volatile * const io_mux_reg, uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num, SemaphoreHandle_t * mutex)
{
    ESP_LOGI(tag, "INIT START");
    
    /* Enable as INPUT */
    *io_mux_reg |= (0x1 << 9);

    params->tag = tag;
    params->ir_gpio_in_reg = ir_gpio_in_reg;
    params->gpio_num = gpio_num;
    params->mutex = mutex;

    ESP_LOGI(tag, "INIT SUCCESSFUL");
}

void servo_init(const char *tag, servo_config_t * srv_cfg, uint8_t speed_mode) 
{
    ESP_LOGI(tag, "INIT START");

    esp_err_t err = iot_servo_init(speed_mode, srv_cfg);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(tag, "INIT SUCCESSFUL");
}

void task_ir_detect(void * pvParameters)
{
    irParams_t * params = (irParams_t *) pvParameters;

    for(;;)
    {
        if (!((*params->ir_gpio_in_reg >> params->gpio_num) & 0x1))
        {
            if (xSemaphoreTake(*params->mutex, portMAX_DELAY) == pdTRUE)
            {
                /* Re-check once mutex is granted */
                if (!((*params->ir_gpio_in_reg >> params->gpio_num) & 0x1))
                {
                    ESP_LOGI(params->tag, "START IR DETECT TASK");
                    servo_open_close(params->ir_gpio_in_reg, SERVO_CALIBRATION_VAL_0, SERVO_CALIBRATION_VAL_180);
                }
                xSemaphoreGive(*params->mutex);
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void ir_wait_for_pet(uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num, uint32_t start_time, uint32_t decrem_time)
{
    // Milliseconds
    uint32_t timer = start_time;

    while (timer > 0)
    {
        vTaskDelay(decrem_time / portTICK_PERIOD_MS);
        timer -= decrem_time;
        if (!((*ir_gpio_in_reg >> gpio_num) & 0x1))
        {
            // Reset timer
            timer = start_time;
        }
    }
}

void servo_open_close(uint32_t volatile * const ir_gpio_in_reg, uint32_t servo_cal_val_0, uint32_t servo_cal_val_180)
{
        iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, servo_cal_val_0);
        
        ir_wait_for_pet(ir_gpio_in_reg, IR_GPIO_NUM, IR_DETECTION_START_TIME_MS, IR_DETECTION_DECREMENT_TIME_MS);
        
        // Slight offset for 60 degrees
        iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, (servo_cal_val_180 / 3) + 10);
}