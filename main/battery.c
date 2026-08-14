#include "battery.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "battery";

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;

esp_err_t battery_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc), TAG, "adc unit");

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, BAT_VBAT_GPIO, &chan_cfg),
                        TAG, "adc channel");

    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
        .default_vref = 1100,
    };
    // Calibration may fail on chips without eFuse data - voltage is then
    // less accurate but the battery % estimate still works.
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali) == ESP_OK)
        return ESP_OK;
    ESP_LOGW(TAG, "no eFuse calibration, using raw readings");
    s_cali = NULL;
    return ESP_OK;
}

esp_err_t battery_read(float *voltage, uint8_t *percent)
{
    int raw = 0;
    ESP_RETURN_ON_ERROR(adc_oneshot_read(s_adc, BAT_VBAT_GPIO, &raw), TAG, "adc read");

    int mv = 0;
    if (s_cali && adc_cali_raw_to_voltage(s_cali, raw, &mv) == ESP_OK) {
        *voltage = (float)mv / 1000.0f / BAT_DIVIDER_RATIO;
    } else {
        // Fallback: rough 3.3V full-scale conversion
        *voltage = (float)raw / 4095.0f * 3.3f / BAT_DIVIDER_RATIO;
    }

    float pct = (*voltage - BAT_V_MIN) / (BAT_V_MAX - BAT_V_MIN) * 100.0f;
    if (pct < 0.0f)
        pct = 0.0f;
    if (pct > 100.0f)
        pct = 100.0f;
    *percent = (uint8_t)pct;

    return ESP_OK;
}