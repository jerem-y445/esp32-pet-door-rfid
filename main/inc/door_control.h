/**
 * door_control.h
 *
 * Definitions for servo motor and IR break beam
 * 
 */

#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#include "inc/main.h"

typedef struct
{
    const char * tag;
    SemaphoreHandle_t * mutex;
    uint32_t volatile * ir_gpio_in_reg; 
    uint8_t gpio_num;
} irParams_t;

/**
 * Definitions
 * 
 */
#define SERVO_PIN           (5)
#define SERVO_MAX_ANGLE     (180)
#define SERVO_MIN_WIDTH     (500)
#define SERVO_MAX_WIDTH     (2400)
#define SERVO_FREQ          (50)
#define SERVO_TIMER_NUM     (LEDC_TIMER_0)
#define SERVO_CHANNEL       (LEDC_CHANNEL_0)
#define SERVO_CHANNEL_NUM   (1)
#define SERVO_SPEED_MODE    (LEDC_LOW_SPEED_MODE)

// Calibration values
#define SERVO_CALIBRATION_VAL_0     (20)
#define SERVO_CALIBRATION_VAL_180   (200)

#define IR_GPIO_NUM         (6)

// IR Detection Start Time
#define IR_START_TIME       (5000)

/**
 * @brief Initialize servo using iot_servo_init function in "iot_servo.h"
 *
 * @param tag Servo tag for debugging
 * @param srv_cfg Pointer to servo config struct
 * @param speed_mode ledc speed mode (LEDC_LOW_SPEED_MODE or LEDC_SPEED_MODE_MAX in "ledc_types.h")
 *
 */
void servo_init(const char *tag, servo_config_t * srv_cfg, uint8_t speed_mode);

/**
 * @brief Initialize IR break beam register of choice
 *
 * @param tag IR tag for debugging
 * @param io_mux_reg Pointer to IO MUX register
 *
 */
void ir_init(const char *tag, irParams_t * params, uint32_t volatile * const io_mux_reg, SemaphoreHandle_t * mutex, uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num);


void ir_wait_for_cat(uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num);


void task_ir_detect(void * pvParameters);


void servo_open_close(uint32_t volatile * const ir_gpio_in_reg, uint32_t servo_cal_val_0, uint32_t servo_cal_val_180);

#endif