#include <string.h>

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "esp_nimble_hci.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

#include "battery.h"
#include "bthome.h"
#include "sensor.h"

static const char *TAG = "froggy";

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------
#define CADENCE_S 300          // deep sleep between broadcasts (s)
#define ADV_DURATION_MS 10000  // how long the BTHome advertisement runs
#define WAKE_GPIO GPIO_NUM_13  // optional button: force an immediate cycle
#define SENSOR_READ_RETRIES 3

// ------------------------------------------------------------
// BTHome advertisement
// ------------------------------------------------------------
static uint8_t s_adv_payload[BTHOME_PAYLOAD_MAX + 2]; // [UUID][payload]
static size_t s_adv_payload_len;

static int gap_event(struct ble_gap_event *event, void *arg)
{
    return 0;
}

static void ble_start_advertising(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.svc_data_uuid16 = s_adv_payload;
    fields.svc_data_uuid16_len = s_adv_payload_len; // includes the 2-byte UUID
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, ADV_DURATION_MS, &adv_params,
                      gap_event, NULL);
}

// ------------------------------------------------------------
// NimBLE host task
// ------------------------------------------------------------
static void host_task(void *param)
{
    nimble_port_run(); // blocks until nimble_port_stop()
    nimble_port_freertos_deinit();
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
void app_main(void)
{
    // NVS is only used by NimBLE (bonding/address); handle first-boot resize
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    // Wake reason (GPIO button = debug force-wake; the cycle below runs anyway)
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT1)
        ESP_LOGW(TAG, "wakeup: button");

    ESP_ERROR_CHECK(sensor_init());
    ESP_ERROR_CHECK(battery_init());

    float temp = 0.0f, hum = 0.0f;
    esp_err_t err = ESP_ERR_TIMEOUT;
    for (int i = 0; i < SENSOR_READ_RETRIES && err != ESP_OK; i++)
        err = sensor_read(&temp, &hum);
    if (err != ESP_OK)
        ESP_LOGW(TAG, "sensor read failed after %d tries", SENSOR_READ_RETRIES);

    float voltage = 0.0f;
    uint8_t percent = 0;
    battery_read(&voltage, &percent);
    ESP_LOGW(TAG, "t=%.2f h=%.2f v=%.3f bat=%u%%", temp, hum, voltage, percent);

    // Build adv payload: service data starts with the BTHome UUID (LE)
    size_t plen = bthome_build(s_adv_payload + 2, BTHOME_PAYLOAD_MAX,
                               temp, hum, percent, voltage);
    s_adv_payload[0] = (uint8_t)(BTHOME_SVC_UUID & 0xFF);
    s_adv_payload[1] = (uint8_t)(BTHOME_SVC_UUID >> 8);
    s_adv_payload_len = plen + 2;

    // NimBLE host
    ESP_ERROR_CHECK(nimble_port_init());
    ble_svc_gap_device_name_set("froggy");
    nimble_port_freertos_init(host_task);

    while (!ble_hs_is_enabled())
        vTaskDelay(pdMS_TO_TICKS(10));

    // Ensure the controller MAC is available as the public address
    ble_hs_util_ensure_addr(0);

    ble_start_advertising();
    ESP_LOGW(TAG, "advertising %d ms", ADV_DURATION_MS);
    vTaskDelay(pdMS_TO_TICKS(ADV_DURATION_MS));
    ble_gap_adv_stop();

    // Cut sensor power: 0 uA in deep sleep
    sensor_power_off();

    // Schedule next wake (and optionally the button). On ESP32, deep sleep
    // GPIO wakeup goes through the EXT1 RTC interface (GPIO13 is RTC_GPIO13).
    esp_sleep_enable_timer_wakeup((uint64_t)CADENCE_S * 1000000ULL);
    esp_sleep_enable_ext1_wakeup(1ULL << WAKE_GPIO, ESP_EXT1_WAKEUP_ANY_HIGH);

    ESP_LOGW(TAG, "deep sleep %d s", CADENCE_S);
    esp_deep_sleep_start();
}