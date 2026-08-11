#include "inc/main.h"

void servo_init(const char *tag, servo_config_t * srv_cfg, uint8_t speed_mode) 
{
    esp_err_t err;

    ESP_LOGI(tag, "INIT SERVO CONTROL");

    err = iot_servo_init(speed_mode, srv_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGI(tag, "FAILED TO INIT SERVO");
    }
}

void ir_init(const char *tag, irParams_t * params, uint32_t volatile * const io_mux_reg, SemaphoreHandle_t * mutex, uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num)
{
    ESP_LOGI(tag, "IR BREAK BEAM INIT");
    *io_mux_reg |= (0x1 << 9);

    params->tag = tag;
    params->mutex = mutex;
    params->ir_gpio_in_reg = ir_gpio_in_reg;
    params->gpio_num = gpio_num;
}

void ir_wait_for_cat(uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num)
{
    uint16_t timer = IR_START_TIME; // in milliseconds

    while (timer > 0)
    {
        vTaskDelay(250 / portTICK_PERIOD_MS);
        timer -= 250; // 
        if (!((*ir_gpio_in_reg >> gpio_num) & 0x1))
        {
            timer = IR_START_TIME; // Reset timer
        }
    }
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
                if (!((*params->ir_gpio_in_reg >> params->gpio_num) & 0x1))
                {
                    ESP_LOGI(params->tag, "ENTERED IR DETECT TASK");
                    servo_open_close(params->ir_gpio_in_reg, SERVO_CALIBRATION_VAL_0, SERVO_CALIBRATION_VAL_180);
                }
                xSemaphoreGive(*params->mutex);
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void servo_open_close(uint32_t volatile * const ir_gpio_in_reg, uint32_t servo_cal_val_0, uint32_t servo_cal_val_180)
{
        // Slight offset for 90 degrees
        iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, servo_cal_val_0);
        ir_wait_for_cat(ir_gpio_in_reg, IR_GPIO_NUM);
        iot_servo_write_angle(SERVO_SPEED_MODE, SERVO_CHANNEL, (servo_cal_val_180 / 3) + 10);
}