#include "aht20.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "aht20";

static i2c_master_dev_handle_t s_dev;

static esp_err_t write_cmd(const uint8_t *cmd, size_t len)
{
    return i2c_master_transmit(s_dev, cmd, len, pdMS_TO_TICKS(100));
}

esp_err_t aht20_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT20_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev),
                        TAG, "add device");

    uint8_t init = AHT20_CMD_INIT;
    ESP_RETURN_ON_ERROR(write_cmd(&init, 1), TAG, "soft reset");
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t cal[] = {AHT20_CMD_CALIBRATE, 0x08, 0x00};
    ESP_RETURN_ON_ERROR(write_cmd(cal, sizeof(cal)), TAG, "calibrate");
    vTaskDelay(pdMS_TO_TICKS(20));

    return ESP_OK;
}

// Returns temperature (C) and humidity (%RH).
// raw: 20-bit values split across 6 status bytes
//   hum_raw = (b[1] << 12) | (b[2] << 4) | (b[3] >> 4)
//   temp_raw = ((b[3] & 0x0F) << 16) | (b[4] << 8) | b[5]
esp_err_t aht20_read(float *temp, float *hum)
{
    uint8_t meas[] = {AHT20_CMD_MEASURE, 0x33, 0x00};
    ESP_RETURN_ON_ERROR(write_cmd(meas, sizeof(meas)), TAG, "measure cmd");
    vTaskDelay(pdMS_TO_TICKS(AHT20_MEASURE_MS));

    uint8_t b[6] = {0};
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_dev, b, sizeof(b), pdMS_TO_TICKS(100)),
                        TAG, "read data");

    if (b[0] & 0x80) {
        ESP_LOGW(TAG, "sensor busy");
        return ESP_ERR_TIMEOUT;
    }

    uint32_t hum_raw = ((uint32_t)b[1] << 12) | ((uint32_t)b[2] << 4) | (b[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(b[3] & 0x0F) << 16) | ((uint32_t)b[4] << 8) | b[5];

    *hum = (float)hum_raw * 100.0f / (1 << 20);
    *temp = (float)temp_raw * 200.0f / (1 << 20) - 50.0f;

    return ESP_OK;
}