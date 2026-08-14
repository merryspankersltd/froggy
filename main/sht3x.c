#include "sht3x.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sht3x";

static i2c_master_dev_handle_t s_dev;

// CRC-8, polynomial 0x31, init 0xFF (Sensirion convention)
static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

esp_err_t sht3x_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT3X_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev),
                        TAG, "add device");
    return ESP_OK;
}

esp_err_t sht3x_read(float *temp, float *hum)
{
    uint8_t cmd[] = {SHT3X_CMD_MEASURE_HIGH, SHT3X_CMD_DATA};
    ESP_RETURN_ON_ERROR(i2c_master_transmit(s_dev, cmd, sizeof(cmd), pdMS_TO_TICKS(100)),
                        TAG, "measure cmd");
    vTaskDelay(pdMS_TO_TICKS(SHT3X_MEASURE_MS));

    // temp (2) + crc, hum (2) + crc
    uint8_t b[6] = {0};
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_dev, b, sizeof(b), pdMS_TO_TICKS(100)),
                        TAG, "read data");

    if (crc8(&b[0], 2) != b[2] || crc8(&b[3], 2) != b[5]) {
        ESP_LOGW(TAG, "crc mismatch");
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t temp_raw = ((uint16_t)b[0] << 8) | b[1];
    uint16_t hum_raw = ((uint16_t)b[3] << 8) | b[4];

    *temp = (float)temp_raw * 175.0f / 65535.0f - 45.0f;
    *hum = (float)hum_raw * 100.0f / 65535.0f;

    return ESP_OK;
}