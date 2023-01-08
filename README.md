# ESP-IDF Wireguard example

Uses:

- [`esp_wireguard`](https://github.com/trombik/esp_wireguard/) component for Wireguard implementation
- `wifi_provisioning` for WiFi provisioning - use Espressif's [`ESP SoftAP Prov`](https://github.com/trombik/esp_wireguard/) Android/iOS app or Python tool to connect ESP32 to WiFi

Based on example from [`esp_wireguard`](https://github.com/trombik/esp_wireguard/) repo.
Removed unnecessary code - may only work with ESP-IDF v4.4+.
Tested with ESP-IDF version `release/v4.4.3`.

## Usage

- Export ESP-IDF
- Clone this repository and enter it
- `git submodule update --init --recursive`
- Configure the example: `idf.py menuconfig` -> Example config
- `idf.py build flash monitor`

## Tips

### Time synchronization

You can find time zone strings needed for time sync here: [nayarsystems/posix_tz_db/blob/master/zones.csv](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)
