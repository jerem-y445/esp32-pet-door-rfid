/**
 * @file    door_control.h
 * Definitions for servo motor and IR break-beam-related functions
 * 
 */

#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#include "inc/main.h"

typedef struct
{
    const char * tag;
    uint32_t volatile * ir_gpio_in_reg; 
    uint8_t gpio_pin_num;
    SemaphoreHandle_t * mutex;
} irParams_t;

/* IR Register Defines
 *   First enable IO MUX for GPIO 6 to be in input mode
 *   Then, read from 6th bit position from GPIO input register
 */
extern uint32_t volatile * const IR_IO_MUX_GPIO6_REG;
extern uint32_t volatile * const IR_GPIO_IN_REG;

/* IR GPIO Number Define */
#define IR_GPIO_NUM         (6)

/* IR Detection Time Defines */
#define IR_DETECTION_START_TIME_MS      (5000)
#define IR_DETECTION_DECREMENT_TIME_MS  (100)

/* Servo Config Defines */
#define SERVO_PIN           (5)
#define SERVO_MAX_ANGLE     (180)
#define SERVO_MIN_WIDTH     (500)
#define SERVO_MAX_WIDTH     (2400)
#define SERVO_FREQ          (50)
#define SERVO_TIMER_NUM     (LEDC_TIMER_0)
#define SERVO_CHANNEL       (LEDC_CHANNEL_0)
#define SERVO_CHANNEL_NUM   (1)
#define SERVO_SPEED_MODE    (LEDC_LOW_SPEED_MODE)

/* Servo Calibration Values */
#define SERVO_CALIBRATION_VAL_0     (20)
#define SERVO_CALIBRATION_VAL_180   (200)

/**
 * @brief Initialize IR break beam register of choice
 *
 * @param tag IR tag for debugging
 * @param params Param struct with irParams_t
 * @param io_mux_reg Pointer to IO MUX register
 * @param ir_gpio_in_reg Pointer to specific GPIO input register
 * @param gpio_num Specific GPIO number to probe
 * @param mutex Used by both task_rfid_detect and task_ir_detect
 *
 */
void task_ir_init(const char *tag, irParams_t * params, uint32_t volatile * const io_mux_reg, uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num, SemaphoreHandle_t * mutex);

/**
 * @brief Initialize servo using iot_servo_init function in "iot_servo.h"
 *
 * @param tag Servo tag for debugging
 * @param srv_cfg Pointer to servo config struct
 * @param speed_mode LEDC speed mode (LEDC_LOW_SPEED_MODE or LEDC_SPEED_MODE_MAX in "ledc_types.h")
 *
 */
void servo_init(const char *tag, servo_config_t * srv_cfg, uint8_t speed_mode);

/**
 * @brief Probes for pet obstruction and unlocks door until the pet moves away. Disables RFID detection task.
 * 
 * @param pvParameters Params found in irParams_t struct
 * 
 */
void task_ir_detect(void * pvParameters);

/**
 * @brief Probes IR beam for pet and resets timer when beam is broken to IR_START_TIME define.
 * 
 * @param ir_gpio_in_reg Pointer to specific GPIO input register
 * @param gpio_num Specific GPIO number to probe
 * @param start_time Specific to IR_START_TIME_MS define
 * @param decrem_time Specific to IR_DECREMENT_TIME_MS define
 * 
 */
void ir_wait_for_pet(uint32_t volatile * const ir_gpio_in_reg, uint8_t gpio_num, uint32_t start_time, uint32_t decrem_time);

/**
 * @brief Turns servo 0 degrees (unlocked state) as fast as SERVO_SPEED_MODE and waits for pet to leave to rotate back to 60 degrees (locked state)
 * 
 * @param ir_gpio_in_reg Pointer to specific GPIO input register
 * @param servo_cal_val_0 Specific to SERVO_CALIBRATION_VAL_0 define
 * @param servo_cal_val_180 Specific to SERVO_CALIBRATION_VAL_180 define
 * 
 */
void servo_open_close(uint32_t volatile * const ir_gpio_in_reg, uint32_t servo_cal_val_0, uint32_t servo_cal_val_180);

#endif