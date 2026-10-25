/**
 * @file lvgl_port_disp.c
 * @brief LVGL 显示 flush -> ST7789V SPI
 *
 * 字节序/分块模式照 main/lcd_blit.c 的 LCD_WritePixels:
 * 主机小端 RGB565 -> 逐像素 swap 成大端发送 (与 LCD_Fill 一致)。
 * 切勿用 LCD_ShowPicture: 其地址窗差 1 像素且不做字节序转换。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lcd.h"
#include "lvgl_port.h"

#define FLUSH_CHUNK_PX      1024            /* 单次 SPI 传输像素 (2048B < max_transfer_sz 4096) */

static const char *TAG = "lvgl_port";

static uint16_t *s_draw_buf;                /* LVGL draw buffer (PSRAM) */
static uint16_t s_swap_buf[FLUSH_CHUNK_PX]; /* 字节交换暂存 (内部 RAM, DMA 可用) */

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t w  = area->x2 - area->x1 + 1;
    int32_t h  = area->y2 - area->y1 + 1;

    if (w <= 0 || h <= 0) {
        lv_display_flush_ready(disp);
        return;
    }

    LCD_Address_Set((uint16_t)x1, (uint16_t)y1,
                    (uint16_t)(x1 + w - 1), (uint16_t)(y1 + h - 1));   /* 含端点 */
    LCD_DC_Set();

    const uint16_t *p = (const uint16_t *)px_map;
    size_t remain = (size_t)w * (size_t)h;
    unsigned chunk_n = 0;
    while (remain > 0) {
        size_t n = remain < FLUSH_CHUNK_PX ? remain : FLUSH_CHUNK_PX;
        for (size_t i = 0; i < n; i++) {
            s_swap_buf[i] = (uint16_t)((p[i] << 8) | (p[i] >> 8));     /* 大端发送序 */
        }
        spi_transaction_t t = {
            .length = n * sizeof(uint16_t) * 8,
            .tx_buffer = s_swap_buf,
        };
        spi_device_polling_transmit(spi, &t);
        p += n;
        remain -= n;
        /* 长块推流是 busy-wait, 周期放一拍给 IDLE, 防 task_wdt */
        if ((++chunk_n & 0x3F) == 0) {
            vTaskDelay(1);
        }
    }

    lv_display_flush_ready(disp);
}

lv_display_t *lvgl_port_disp_create(uint16_t hres, uint16_t vres, uint16_t buf_height)
{
    uint32_t buf_px = (uint32_t)hres * buf_height;
    s_draw_buf = heap_caps_malloc(buf_px * sizeof(uint16_t),
                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_draw_buf == NULL) {
        ESP_LOGE(TAG, "draw buffer alloc failed (%u px)", (unsigned)buf_px);
        return NULL;
    }

    lv_display_t *disp = lv_display_create(hres, vres);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, s_draw_buf, NULL,
                           buf_px * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    return disp;
}
