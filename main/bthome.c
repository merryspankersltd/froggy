#include "bthome.h"

size_t bthome_build(uint8_t *buf, size_t cap, float temp, float hum,
                    uint8_t battery, float voltage)
{
    if (cap < BTHOME_PAYLOAD_MAX)
        return 0;

    size_t p = 0;
    buf[p++] = BTHOME_DEVICE_INFO;

    // Temperature: sint16, 0.01 C
    int16_t t = (int16_t)(temp * 100.0f + (temp >= 0 ? 0.5f : -0.5f));
    buf[p++] = BTHOME_ID_TEMP;
    buf[p++] = (uint8_t)(t & 0xFF);
    buf[p++] = (uint8_t)((t >> 8) & 0xFF);

    // Humidity: uint16, 0.01 %
    uint16_t h = (uint16_t)(hum * 100.0f + 0.5f);
    buf[p++] = BTHOME_ID_HUM;
    buf[p++] = (uint8_t)(h & 0xFF);
    buf[p++] = (uint8_t)((h >> 8) & 0xFF);

    // Battery: uint8, %
    buf[p++] = BTHOME_ID_BATTERY;
    buf[p++] = battery;

    // Voltage: uint16, 0.001 V
    uint16_t v = (uint16_t)(voltage * 1000.0f + 0.5f);
    buf[p++] = BTHOME_ID_VOLTAGE;
    buf[p++] = (uint8_t)(v & 0xFF);
    buf[p++] = (uint8_t)((v >> 8) & 0xFF);

    return p;
}