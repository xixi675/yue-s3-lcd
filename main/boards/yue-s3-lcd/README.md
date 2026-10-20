# Yue S3 LCD

Custom ESP32-S3 voice assistant board derived from
`bread-compact-wifi-lcd`.

## Hardware

- ESP32-S3-WROOM-1-N16R8: 16 MB quad SPI flash, 8 MB octal PSRAM
- Wi-Fi + BLE 5
- Wi-Fi configuration hotspot prefix: `xiaoyue`
- ST7789 170x320 SPI LCD
- Direct I2S microphone and speaker (no external audio codec)
- BOOT button on GPIO0, status LED on GPIO48
- Optional lamp output on GPIO18

## Pin Map

| Function | GPIO |
| --- | --- |
| Microphone WS / SCK / DIN | 4 / 5 / 6 |
| Speaker DOUT / BCLK / LRCK | 7 / 15 / 16 |
| LCD MOSI / CLK / DC / RST / CS / Backlight | 47 / 21 / 40 / 45 / 41 / 42 |
| BOOT button | 0 |
| Status LED | 48 |
| Lamp output | 18 |

## Build

```sh
python scripts/build.py yue-s3-lcd --name yue-s3-lcd
```

The board reports `yue-s3-lcd` as its OTA identity, so it does not share the
official `bread-compact-wifi-lcd` OTA channel.
