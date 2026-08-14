#pragma once

#include "esp_err.h"

#define BAT_VBAT_GPIO ADC_CHANNEL_6 // GPIO34, ADC1_CH6

// VBAT divider: 100 kOhm top (VBAT -> GPIO), 47 kOhm bottom (GPIO -> GND)
#define BAT_DIVIDER_RATIO (47000.0f / (100000.0f + 47000.0f))

// Li-Ion curve endpoints for battery % estimation (linear)
#define BAT_V_MIN 3.0f // 0 %
#define BAT_V_MAX 4.2f // 100 %

esp_err_t battery_init(void);
// Returns battery voltage (V) and estimated charge (0-100 %)
esp_err_t battery_read(float *voltage, uint8_t *percent);