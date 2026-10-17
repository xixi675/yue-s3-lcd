# yue-s3-lcd Firmware

Build output for the `yue-s3-lcd` board. These files are local build
artifacts and are intentionally ignored by Git (`*.bin`).

- `merged-binary.bin`: complete flash image for a 16 MB ESP32-S3, flash at `0x0`
- `yue-s3-lcd.bin`: application-only image, flash at `0x20000`
- `bootloader.bin`: bootloader, flash at `0x0`
- `partition-table.bin`: partition table, flash at `0x8000`
- `ota_data_initial.bin`: initial OTA data, flash at `0xd000`
- `generated_assets.bin`: assets partition image, flash at `0x800000`

Command-line flash example:

```sh
python -m esptool --chip esp32s3 -b 460800 \
  --before default-reset --after hard-reset write-flash \
  --flash-mode dio --flash-size 16MB --flash-freq 80m \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0xd000 ota_data_initial.bin \
  0x20000 yue-s3-lcd.bin \
  0x800000 generated_assets.bin
```
