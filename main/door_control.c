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

void ir_init(const char *tag, uint32_t volatile * const io_mux_reg)
{
    *io_mux_reg |= (0x1 << 9);
    ESP_LOGI(tag, "IR BREAK BEAM INIT");
}

void ir_wait_for_cat(const char * tag, uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num)
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
        ESP_LOGI(tag, "timer: %d", timer);
    }
}