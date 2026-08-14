#pragma once

// ============================================================
// SENSOR SELECTION - uncomment EXACTLY ONE
// ============================================================
#define SENSOR_AHT20
// #define SENSOR_SHT3X
// ============================================================

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#define SENSOR_POWER_GPIO GPIO_NUM_23 // sensor power rail (0 uA in deep sleep)
#define SENSOR_SDA_GPIO GPIO_NUM_21
#define SENSOR_SCL_GPIO GPIO_NUM_22

esp_err_t sensor_init(void);
esp_err_t sensor_read(float *temp, float *hum);
void sensor_power_off(void); // rail off before deep sleep (0 uA)