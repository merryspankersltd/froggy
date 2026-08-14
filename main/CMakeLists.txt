idf_component_register(
    SRCS "main.c" "bthome.c" "sensor.c" "aht20.c" "sht3x.c" "battery.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash bt esp_nimble nimble esp_adc driver esp_deep_sleep
)