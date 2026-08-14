#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#define AHT20_ADDR 0x38
#define AHT20_CMD_INIT 0xBE
#define AHT20_CMD_CALIBRATE 0xE1
#define AHT20_CMD_MEASURE 0xAC
#define AHT20_MEASURE_MS 90

esp_err_t aht20_init(i2c_master_bus_handle_t bus);
esp_err_t aht20_read(float *temp, float *hum);