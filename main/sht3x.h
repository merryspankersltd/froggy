#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#define SHT3X_ADDR 0x44
#define SHT3X_CMD_MEASURE_HIGH 0x2C // high repeatability, clock stretching off
#define SHT3X_CMD_DATA 0x06
#define SHT3X_MEASURE_MS 20

esp_err_t sht3x_init(i2c_master_bus_handle_t bus);
esp_err_t sht3x_read(float *temp, float *hum);