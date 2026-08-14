# Froggy

![status](https://img.shields.io/badge/status-experimental-orange)

Battery-powered **BLE temperature/humidity sensor** (ESP32-WROOM-32D, BTHome v2, deep sleep), picked up by an existing ESPHome Bluetooth proxy and Home Assistant's built-in BTHome integration.

This is the step forward after the classic *ESPHome + DHT22 over WiFi* integration: no ESPHome, no WiFi, no OTA — a bare ESP-IDF firmware, flash-once, optimized for battery life.

> **Experimental.** Pre-coded for I2C sensors (AHT20/AHT21 or SHT30/SHT31) — **not DHT22**. No new sensor is sourced yet, so this firmware has not been flashed or measured on hardware.

## Testing status

### Validated on hardware (via the ESPHome prototype, same board)

- BTHome v2 broadcast picked up by the Bluetooth proxy and HA's BTHome integration
- Wake cadence (device wakes every ~60 s, arrivals in multiples of ~60 s)
- Deep sleep + periodic wake cycle
- Serial flashing procedure (CH340 over USB)
- DHT22 reads (temperature/humidity)
- Signal improvement from tx_power/retransmit tuning: behind-wall arrival rate measured at 15% → 33%, median gap 297 s → 170 s
- RSSI behind a wall: ~-81 dBm (old firmware) / ~-89 dBm (new firmware)

### NOT tested (froggy_ble firmware)

- The firmware **compiles clean** with ESP-IDF v5.4.1 (verified), but has never been flashed or run on hardware
- AHT20/SHT30 I2C drivers — no sensor sourced
- Battery ADC + divider (GPIO34) and battery % estimation
- Sensor power rail switching (GPIO23)
- GPIO13 wake button
- Actual deep-sleep current and battery runtime (power budget above is estimated)
- NimBLE advertising from this codebase (only the ESPHome/BTHome path was validated)

## Hardware

- ESP32-WROOM-32D (any classic ESP32 dev board without a noisy regulator for battery use — see Power)
- I2C temperature/humidity sensor, exactly one of:
  - **AHT20/AHT21** (addr `0x38`)
  - **SHT30/SHT31** (addr `0x44`)
- 2x resistors for the VBAT voltage divider (100 kΩ / 47 kΩ)
- Battery: 18650 Li-Ion or LiPo (optional solar panel)

### Wiring

| Signal | ESP32 |
|--------|-------|
| SDA    | GPIO21 |
| SCL    | GPIO22 |
| Sensor power rail | GPIO23 (turns the sensor fully off in deep sleep) |
| VBAT divider (100 kΩ → GPIO, 47 kΩ → GND) | GPIO34 (ADC1_CH6) |
| Wake button (optional, GPIO13 to 3.3V) | GPIO13 (forces an immediate broadcast) |

I2C breakouts need pull-ups (most modules have them built in). The sensor **must** be on a GPIO-switched rail — its quiescent current would otherwise dominate the power budget.

## Power budget

One wake cycle (5 min cadence): sensor rail on ~200 ms (I2C read), broadcast ~10 s, then deep sleep.

| Component | Current |
|-----------|---------|
| Deep sleep (ESP32 + low-quiescent LDO) | ~22-25 µA |
| I2C sensor (rail off in sleep) | 0 µA |
| Wake burst (boot + read + adv), amortized over 300 s | +~0.06 µA |
| **Average** | **~25 µA** |

| Battery | Expected runtime |
|---------|------------------|
| 18650 (3000 mAh) | ~5 yr (self-discharge-limited) |
| 700 mAh LiPo | ~3 yr |
| Solar + small panel | sustainable (needs only ~0.6 mAh/day) |

Notes:
- The DHT22 route would add 50-80 µA quiescent (or a 2.5 s wake at 1.2 mA) — ~3-10x the entire sleep budget. I2C + power rail is the whole point.
- Devkit regulators (AMS1117) draw ~5 mA idle and need ≥4.4 V: replace with HT7833 (3 µA Iq) or TPS62742 (360 nA Iq, full 3.0-4.2 V range) for battery operation.
- Wake cadence is configurable (`CADENCE_S` in `main/main.c`, default 300 s).

## Firmware

Bare ESP-IDF (no ESPHome, no WiFi, no OTA — reflashing requires the serial cable, by design).

```
main/
├── main.c          boot → power rail on → read → BTHome advertise → deep sleep
├── bthome.c/.h     BTHome v2 payload encoder (temp, humidity, battery %/voltage)
├── sensor.c/.h     sensor dispatch + selection blocks (AHT20 vs SHT3X)
├── aht20.c/.h      hand-rolled I2C driver (0x38)
├── sht3x.c/.h      hand-rolled I2C driver (0x44)
└── battery.c/.h    VBAT divider read + Li-Ion % estimation
```

### Sensor selection

Uncomment exactly one block in `main/sensor.h`:

```c
// SENSOR SELECTION — uncomment EXACTLY ONE
#define SENSOR_AHT20
// #define SENSOR_SHT3X
```

### Behavior

- BTHome v2 advertisement (service UUID `0xFCD2`, unencrypted): temperature (0.01 °C), humidity (0.01 %), battery percentage, battery voltage
- Deep sleep between broadcasts; `CADENCE_S` default 300 s
- GPIO13 wake (button): broadcasts immediately, then resumes the schedule — useful for debugging
- Optional power rail: sensor fully off in deep sleep (0 µA)

### Flash

Requires ESP-IDF v5.2+:

```sh
idf.py build
idf.py -p /dev/ttyUSB0 flash
```

Flash once and forget — there is no OTA.

## Prerequisites

- An ESPHome **Bluetooth proxy** in range (froggy is a pure BLE broadcaster)
- Home Assistant with the built-in **BTHome** integration (auto-discovers the device, no config needed)

## License

GNU GPL v3 — see [LICENSE](LICENSE).
