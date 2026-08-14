#include "sensor.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "aht20.h"
#include "sht3x.h"

static const char *TAG = "sensor";

static i2c_master_bus_handle_t s_bus;

esp_err_t sensor_init(void)
{
    // Power the sensor rail so it can answer I2C
    gpio_config_t rail = {
        .pin_bit_mask = 1ULL << SENSOR_POWER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&rail), TAG, "rail gpio config");
    ESP_RETURN_ON_ERROR(gpio_set_level(SENSOR_POWER_GPIO, 1), TAG, "rail on");

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SENSOR_SDA_GPIO,
        .scl_io_num = SENSOR_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c bus");

    vTaskDelay(pdMS_TO_TICKS(50)); // let the sensor power up

#if defined(SENSOR_AHT20)
    return aht20_init(s_bus);
#elif defined(SENSOR_SHT3X)
    return sht3x_init(s_bus);
#else
#error "no sensor selected - uncomment one in sensor.h"
#endif
}

esp_err_t sensor_read(float *temp, float *hum)
{
#if defined(SENSOR_AHT20)
    return aht20_read(temp, hum);
#elif defined(SENSOR_SHT3X)
    return sht3x_read(temp, hum);
#endif
}

// Turn the rail off before deep sleep so the sensor draws 0 uA
void sensor_power_off(void)
{
    gpio_set_level(SENSOR_POWER_GPIO, 0);
}