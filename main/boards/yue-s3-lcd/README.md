# Yue S3 LCD

Custom ESP32-S3 voice assistant board derived from
`bread-compact-wifi-lcd`.

## Hardware

- ESP32-S3-WROOM-1-N16R8: 16 MB quad SPI flash, 8 MB octal PSRAM
- Wi-Fi + BLE 5
- Wi-Fi configuration hotspot prefix: `xiaoyue`
- ST7789 320x170 landscape SPI IPS TFT (T190X7-C30-01)
- Direct I2S microphone and speaker (no external audio codec)
- BOOT button on GPIO48, status LED on GPIO1 (active low)
- Optional lamp output on GPIO14

## Pin Map

| Function | GPIO |
| --- | --- |
| Microphone WS / SCK / DIN | 4 / 5 / 6 |
| Speaker DOUT / BCLK / LRCK | 7 / 15 / 16 |
| LCD MOSI / CLK / DC / RST / CS / Backlight | 47 / 21 / 40 / 45 / 41 / 42 |
| BOOT button (active low) | 48 |
| Volume up / Volume down (active low) | 20 / 19 |
| Status LED (active low) | 1 |
| Second indicator LED (active low) | 2 |
| Lamp output | 14 |

## Build

```sh
python scripts/build.py yue-s3-lcd --name yue-s3-lcd
```

The board reports `yue-s3-lcd` as its OTA identity, so it does not share the
official `bread-compact-wifi-lcd` OTA channel.
