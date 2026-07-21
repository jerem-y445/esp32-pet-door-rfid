#include <esp_log.h>
#include "esp_system.h"
#include "iot_servo.h"

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

