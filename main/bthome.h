#pragma once

#include <stdint.h>
#include <stddef.h>

// BTHome v2 advertisement payload (service data, UUID 0xFCD2, unencrypted).
// Object IDs: temperature 0x01 (sint16, 0.01 C), humidity 0x02 (uint16, 0.01%),
// battery 0x08 (uint8, %), voltage 0x09 (uint16, 0.001 V).

#define BTHOME_SVC_UUID 0xFCD2
#define BTHOME_DEVICE_INFO 0x40 // BTHome v2, unencrypted
#define BTHOME_ID_TEMP 0x01
#define BTHOME_ID_HUM 0x02
#define BTHOME_ID_BATTERY 0x08
#define BTHOME_ID_VOLTAGE 0x09

// Max payload size (device info + all four objects + bounds)
#define BTHOME_PAYLOAD_MAX 16

// Returns payload size, or 0 on failure. temp in deg C, hum in %RH,
// battery in %, voltage in V.
size_t bthome_build(uint8_t *buf, size_t cap, float temp, float hum,
                    uint8_t battery, float voltage);