# Froggy

ESP32-WROOM-32D + DHT22 temperature/humidity sensor broadcasting via BTHome BLE advertisements, picked up by an existing ESPHome Bluetooth proxy and Home Assistant's built-in BTHome integration.

- Data pin: **GPIO21**
- Broadcasts every **1 minute**
- Deep sleep between broadcasts with **NTP drift correction** (each wake re-syncs via SNTP, sleep duration computed to land on the next minute boundary + 5s)

## Wiring

| DHT22 | ESP32 |
|-------|-------|
| VCC   | 3.3V  |
| GND   | GND   |
| DATA  | GPIO21 |

Bare DHT22 sensors need a 10kΩ pull-up resistor between DATA and 3.3V (most modules have it built in).

## Setup

The config expects these keys in the ESPHome instance's `secrets.yaml` (`/config/esphome/secrets.yaml`):

- `wifi_ssid`, `wifi_password` - WiFi credentials
- `api_encryption_key` - base64 API encryption key
- `froggy_ota_password` - per-device OTA password

Flash: `esphome run froggy.yaml`

Home Assistant auto-discovers the device via its BTHome integration (through your Bluetooth proxy) - no proxy config changes needed

## How it works

- Every ~60s the ESP32 wakes, reads the DHT22, broadcasts BTHome advertisements (NimBLE stack, +9dBm, 2-4s adv interval, 2x retransmit), then deep sleeps (~10µA)
- `on_time_sync` (SNTP) computes the sleep duration so the next wake lands at the next minute boundary + 5s, self-correcting RTC drift every cycle
- WiFi/API/OTA stay enabled during the wake window so the device can be reflashed
- Note: the BLE MAC differs from the WiFi MAC by the last byte (e.g. `...:8E` vs `...:8C`) — use the BLE MAC for debugging scans

## Battery migration (future)

`froggy-battery.yaml` is the battery-optimized variant: WiFi/API/OTA removed (flash over serial only), CPU at 80MHz, TX power reduced, fixed 10s wake / 290s sleep cycle (~5 min cadence, no SNTP drift correction).

Expected runtime on a 3000-3350 mAh 18650: **~2.5-3 months** (vs ~5 days in the current WiFi-enabled config).

### Required hardware changes

- **Power path**: the devkit's AMS1117 regulator draws ~5 mA idle and needs ≥4.4V input (an 18650 only provides 3.0-4.2V). Replace it with a low-quiescent regulator feeding the bare 3.3V rail:
  - HT7833 (3 µA Iq) — simple LDO, usable while battery is 3.6-4.2V
  - TPS62742 (360 nA Iq) — buck, full 3.0-4.2V range
- **Sleep current**: expect ~70 µA in deep sleep (ESP32 ~20 µA + DHT22 ~50 µA quiescent)

### Trade-offs vs `froggy.yaml`

- No OTA/API: reflashing requires the serial cable
- No SNTP: RTC drifts ~10-20 s/day, so the 1-min cadence slowly shifts (irrelevant for temperature reporting)
- Lower TX power reduces range (0 dBm is fine at ~1 m from the proxy)