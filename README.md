# Yue S3 LCD

基于 ESP32-S3 的早教机，适配 ESP32-S3-WROOM-1-N16R8 开发板。

## 硬件

| 项目 | 规格 |
| --- | --- |
| 芯片 | ESP32-S3 QFN56，双核 240MHz，Wi-Fi + BLE 5 |
| Flash | 16MB，quad，3.3V |
| PSRAM | 8MB octal，80MHz |
| 模块 | ESP32-S3-WROOM-1-N16R8 |
| 屏幕 | ST7789 170x320 SPI LCD |
| 音频 | 麦克风与扬声器直连 I2S，无独立编解码芯片 |

## 引脚

| 功能 | GPIO |
| --- | --- |
| 麦克风 WS / SCK / DIN | 4 / 5 / 6 |
| 扬声器 DOUT / BCLK / LRCK | 7 / 15 / 16 |
| LCD MOSI / CLK / DC / RST / CS | 47 / 21 / 40 / 45 / 41 |
| LCD 背光 | 42 |
| BOOT 按键 | 0 |
| 状态灯 | 48 |
| 示例灯输出 | 18 |

## 功能

- Wi-Fi 配网，支持热点配网
- 离线语音唤醒，支持 ESP-SR 与自定义唤醒词
- WebSocket 与 MQTT + UDP 两种通信方式
- Opus 音频流，支持流式与实时语音模型
- OLED / LCD 显示，支持表情与聊天界面
- 设备端 MCP 工具，可让 AI 控制 GPIO、灯光、电机等

## 目录

| 路径 | 说明 |
| --- | --- |
| `main/boards/yue-s3-lcd/` | 板卡引脚、初始化代码和板卡说明 |
| `main/boards/common/` | 通用板卡接口和硬件辅助实现 |
| `main/audio/` | 音频编解码、唤醒词、音频服务 |
| `main/display/` | LCD、OLED、LVGL 界面 |
| `main/protocols/` | WebSocket、MQTT + UDP 协议 |
| `main/mcp_server.*` | 设备端 MCP 工具注册与调度 |
| `firmware/` | 本机编译出的可烧录固件 |
| `scripts/` | 编译、资源打包和语言生成脚本 |

## 编译

需要 ESP-IDF v6.0.1 或更高版本，推荐 v6.1。

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
python scripts/build.py yue-s3-lcd --name yue-s3-lcd
```

## 烧录

整机一键烧录：

```powershell
python -m esptool --chip esp32s3 -b 460800 write-flash 0x0 firmware/merged-binary.bin
```

或者使用分区文件烧录：

```powershell
python -m esptool --chip esp32s3 -b 460800 \
  --before default-reset --after hard-reset write-flash \
  --flash-mode dio --flash-size 16MB --flash-freq 80m \
  0x0 firmware/bootloader.bin \
  0x8000 firmware/partition-table.bin \
  0xd000 firmware/ota_data_initial.bin \
  0x20000 firmware/yue-s3-lcd.bin \
  0x800000 firmware/generated_assets.bin
```

## 二次开发

- 修改引脚、屏幕参数：`main/boards/yue-s3-lcd/config.h`
- 添加按键、外设、MCP 工具：`main/boards/yue-s3-lcd/yue_s3_lcd.cc`
- 添加 AI 可调用能力：参考 `main/boards/common/lamp_controller.h`
- 修改界面：`main/display/`
- 修改通信协议：`main/protocols/`

## 许可证

本项目基于 MIT 许可的 `xiaozhi-esp32` 二次开发，原始版权声明和许可证保留在 [LICENSE](LICENSE) 中。
