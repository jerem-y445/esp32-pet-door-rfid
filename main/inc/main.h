#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <esp_log.h>
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

#include "iot_servo.h"
#include "pn532.h"
#include "pn532_driver_i2c.h"

#include "inc/door_control.h"
#include "inc/pn532_rfid.h"

#endif