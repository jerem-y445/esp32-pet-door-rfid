/*
 * door_control.h
 *
 * Definitions for servo motor lock and IR break beam
 */

#include <stdint.h>
#include "iot_servo.h"

#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#define SERVO_PIN           (5)
#define SERVO_MAX_ANGLE     180
#define SERVO_MIN_WIDTH     500
#define SERVO_MAX_WIDTH     2400
#define SERVO_FREQ          50
#define SERVO_TIMER_NUM     LEDC_TIMER_0
#define SERVO_CHANNEL       LEDC_CHANNEL_0
#define SERVO_CHANNEL_NUM   1
#define SERVO_SPEED_MODE    LEDC_LOW_SPEED_MODE

void servo_init(const char *tag, servo_config_t * srv_cfg, uint8_t speed_mode);

void ir_init(const char *tag, uint32_t volatile * const io_mux_reg);

#endif