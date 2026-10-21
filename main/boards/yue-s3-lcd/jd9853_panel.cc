#include "jd9853_panel.h"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_compiler.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_interface.h>
#include <esp_lcd_panel_io.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <sys/cdefs.h>

static const char* TAG = "lcd_panel.jd9853";

typedef struct {
    uint8_t command;
    uint8_t data[32];
    uint8_t data_bytes;
    uint16_t delay_ms;
} jd9853_init_cmd_t;

// BOE 2.01 WV020JAU-N80, JD9853, 240x296.
static const jd9853_init_cmd_t jd9853_init_commands[] = {
    {0x01, {}, 0, 100},
    {0xDF, {0x98, 0x53}, 2, 0},
    {0xDE, {0x00}, 1, 0},
    {0xB2, {0x25}, 1, 0},
    {0xB7, {0x00, 0x21, 0x00, 0x49}, 4, 0},
    {0xBB, {0x4F, 0x1A, 0x55, 0x73, 0x63, 0xF0}, 6, 0},
    {0xC0, {0x44, 0xA4}, 2, 0},
    {0xC1, {0x12}, 1, 0},
    {0xC3, {0x7D, 0x07, 0x14, 0x06, 0xC8, 0x71, 0x6C, 0x77}, 8, 0},
    {0xC4, {0x00, 0x00, 0x94, 0x79, 0x25, 0x0A, 0x16, 0x79, 0x25, 0x0A, 0x16, 0x82}, 12, 0},
    {0xC8,
     {0x3F, 0x34, 0x2B, 0x20, 0x2A, 0x2C, 0x24, 0x24, 0x21, 0x22, 0x20,
      0x15, 0x10, 0x0B, 0x06, 0x00, 0x3F, 0x34, 0x2B, 0x20, 0x2A, 0x2C,
      0x24, 0x24, 0x21, 0x22, 0x20, 0x15, 0x10, 0x0B, 0x06, 0x00},
     32,
     0},
    {0xD0, {0x04, 0x06, 0x6B, 0x0F, 0x00}, 5, 0},
    {0xD7, {0x20, 0x30}, 2, 0},
    {0xE6, {0x14}, 1, 0},
    {0xDE, {0x01}, 1, 0},
    {0xB7, {0x03, 0x13, 0xEF, 0x35, 0x35}, 5, 0},
    {0xC1, {0x14, 0x15, 0xC0}, 3, 0},
    {0xC2, {0x06, 0x3A, 0xC7}, 3, 0},
    {0xC4, {0x72, 0x12}, 2, 0},
    {0xBE, {0x00}, 1, 0},
    {0xDE, {0x00}, 1, 0},
    {0x35, {0x00}, 1, 0},
    {0x3A, {0x05}, 1, 0},
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4, 0},
    {0x2B, {0x00, 0x00, 0x01, 0x27}, 4, 0},
    {0x11, {}, 0, 120},
    {0x29, {}, 0, 1},
    {0x20, {}, 0, 0},
};

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    gpio_num_t reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t madctl_val;
} jd9853_panel_t;

static esp_err_t panel_jd9853_del(esp_lcd_panel_t* panel);
static esp_err_t panel_jd9853_reset(esp_lcd_panel_t* panel);
static esp_err_t panel_jd9853_init(esp_lcd_panel_t* panel);
static esp_err_t panel_jd9853_draw_bitmap(esp_lcd_panel_t* panel, int x_start, int y_start,
                                          int x_end, int y_end, const void* color_data);
static esp_err_t panel_jd9853_invert_color(esp_lcd_panel_t* panel, bool invert_color_data);
static esp_err_t panel_jd9853_mirror(esp_lcd_panel_t* panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_jd9853_swap_xy(esp_lcd_panel_t* panel, bool swap_axes);
static esp_err_t panel_jd9853_set_gap(esp_lcd_panel_t* panel, int x_gap, int y_gap);
static esp_err_t panel_jd9853_disp_on_off(esp_lcd_panel_t* panel, bool on_off);
static esp_err_t panel_jd9853_sleep(esp_lcd_panel_t* panel, bool sleep);

esp_err_t esp_lcd_new_panel_jd9853(const esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t* panel_dev_config,
                                   esp_lcd_panel_handle_t* ret_panel) {
    esp_err_t ret = ESP_OK;
    jd9853_panel_t* jd9853 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG,
                      "invalid argument");
    ESP_GOTO_ON_FALSE(panel_dev_config->bits_per_pixel == 16, ESP_ERR_NOT_SUPPORTED, err, TAG,
                      "JD9853 supports 16-bit RGB565 only");

    jd9853 = static_cast<jd9853_panel_t*>(calloc(1, sizeof(jd9853_panel_t)));
    ESP_GOTO_ON_FALSE(jd9853, ESP_ERR_NO_MEM, err, TAG, "no mem for JD9853 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
            .mode = GPIO_MODE_OUTPUT,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure reset GPIO failed");
    }

    switch (panel_dev_config->rgb_ele_order) {
        case LCD_RGB_ELEMENT_ORDER_RGB:
            jd9853->madctl_val = 0;
            break;
        case LCD_RGB_ELEMENT_ORDER_BGR:
            jd9853->madctl_val = LCD_CMD_BGR_BIT;
            break;
        default:
            ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported RGB order");
    }

    jd9853->io = io;
    jd9853->reset_gpio_num = panel_dev_config->reset_gpio_num;
    jd9853->reset_level = panel_dev_config->flags.reset_active_high;
    jd9853->base.del = panel_jd9853_del;
    jd9853->base.reset = panel_jd9853_reset;
    jd9853->base.init = panel_jd9853_init;
    jd9853->base.draw_bitmap = panel_jd9853_draw_bitmap;
    jd9853->base.invert_color = panel_jd9853_invert_color;
    jd9853->base.mirror = panel_jd9853_mirror;
    jd9853->base.swap_xy = panel_jd9853_swap_xy;
    jd9853->base.set_gap = panel_jd9853_set_gap;
    jd9853->base.disp_on_off = panel_jd9853_disp_on_off;
    jd9853->base.disp_sleep = panel_jd9853_sleep;
    *ret_panel = &jd9853->base;
    ESP_LOGD(TAG, "new JD9853 panel @%p", jd9853);
    return ESP_OK;

err:
    if (jd9853) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(jd9853);
    }
    return ret;
}

static esp_err_t panel_jd9853_del(esp_lcd_panel_t* panel) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    if (jd9853->reset_gpio_num >= 0) {
        gpio_reset_pin(jd9853->reset_gpio_num);
    }
    free(jd9853);
    return ESP_OK;
}

static esp_err_t panel_jd9853_reset(esp_lcd_panel_t* panel) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    if (jd9853->reset_gpio_num >= 0) {
        gpio_set_level(jd9853->reset_gpio_num, jd9853->reset_level);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(jd9853->reset_gpio_num, !jd9853->reset_level);
        vTaskDelay(pdMS_TO_TICKS(100));
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(jd9853->io, LCD_CMD_SWRESET, NULL, 0), TAG,
                            "software reset failed");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return ESP_OK;
}

static esp_err_t panel_jd9853_init(esp_lcd_panel_t* panel) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    for (const auto& command : jd9853_init_commands) {
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_io_tx_param(jd9853->io, command.command,
                                      command.data_bytes ? command.data : NULL, command.data_bytes),
            TAG, "init command 0x%02X failed", command.command);
        if (command.delay_ms) {
            vTaskDelay(pdMS_TO_TICKS(command.delay_ms));
        }
    }
    ESP_RETURN_ON_ERROR(
        esp_lcd_panel_io_tx_param(jd9853->io, LCD_CMD_MADCTL, &jd9853->madctl_val, 1), TAG,
        "set MADCTL failed");
    return ESP_OK;
}

static esp_err_t panel_jd9853_draw_bitmap(esp_lcd_panel_t* panel, int x_start, int y_start,
                                          int x_end, int y_end, const void* color_data) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    x_start += jd9853->x_gap;
    x_end += jd9853->x_gap;
    y_start += jd9853->y_gap;
    y_end += jd9853->y_gap;

    ESP_RETURN_ON_ERROR(
        esp_lcd_panel_io_tx_param(jd9853->io, LCD_CMD_CASET,
                                  (uint8_t[]){(uint8_t)(x_start >> 8), (uint8_t)x_start,
                                              (uint8_t)((x_end - 1) >> 8), (uint8_t)(x_end - 1)},
                                  4),
        TAG, "set CASET failed");
    ESP_RETURN_ON_ERROR(
        esp_lcd_panel_io_tx_param(jd9853->io, LCD_CMD_RASET,
                                  (uint8_t[]){(uint8_t)(y_start >> 8), (uint8_t)y_start,
                                              (uint8_t)((y_end - 1) >> 8), (uint8_t)(y_end - 1)},
                                  4),
        TAG, "set RASET failed");

    size_t length = (size_t)(x_end - x_start) * (y_end - y_start) * 2;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(jd9853->io, LCD_CMD_RAMWR, color_data, length),
                        TAG, "write frame memory failed");
    return ESP_OK;
}

static esp_err_t panel_jd9853_invert_color(esp_lcd_panel_t* panel, bool invert_color_data) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    int command = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;
    return esp_lcd_panel_io_tx_param(jd9853->io, command, NULL, 0);
}

static esp_err_t panel_jd9853_mirror(esp_lcd_panel_t* panel, bool mirror_x, bool mirror_y) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    if (mirror_x) {
        jd9853->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        jd9853->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        jd9853->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        jd9853->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    return esp_lcd_panel_io_tx_param(jd9853->io, LCD_CMD_MADCTL, &jd9853->madctl_val, 1);
}

static esp_err_t panel_jd9853_swap_xy(esp_lcd_panel_t* panel, bool swap_axes) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    if (swap_axes) {
        jd9853->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        jd9853->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    return esp_lcd_panel_io_tx_param(jd9853->io, LCD_CMD_MADCTL, &jd9853->madctl_val, 1);
}

static esp_err_t panel_jd9853_set_gap(esp_lcd_panel_t* panel, int x_gap, int y_gap) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    jd9853->x_gap = x_gap;
    jd9853->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_jd9853_disp_on_off(esp_lcd_panel_t* panel, bool on_off) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    int command = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;
    return esp_lcd_panel_io_tx_param(jd9853->io, command, NULL, 0);
}

static esp_err_t panel_jd9853_sleep(esp_lcd_panel_t* panel, bool sleep) {
    jd9853_panel_t* jd9853 = __containerof(panel, jd9853_panel_t, base);
    int command = sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(jd9853->io, command, NULL, 0), TAG,
                        "sleep command failed");
    if (!sleep) {
        vTaskDelay(pdMS_TO_TICKS(120));
    }
    return ESP_OK;
}
