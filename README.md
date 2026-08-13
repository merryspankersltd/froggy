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

When moving to battery power:
- Disable WiFi on boot: `wifi: enable_on_boot: false` and remove `api`/`ota`/`logger`
- Lower TX power (`bthome: tx_power: -6`)
- Optionally cap CPU at 80MHz (`CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_80`)