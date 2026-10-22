/**
 * @file lcd_blit.c
 * @brief LCD 视频推屏适配层实现
 *
 * 复用 lcd_touch 组件的 SPI 句柄 (extern spi) 与 LCD_Address_Set,
 * 字节序转换规则与 LCD_Fill 一致 (主机小端 RGB565 → 大端发送)。
 * 所有绘制经同一互斥锁: lcd_render 线程推帧与解码线程画状态字可并发。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "lcd_blit.h"

#define BLIT_CHUNK_PX       1024                /* 单次 SPI 传输像素数 (2048B < max_transfer_sz 4096) */
#define BLIT_SHOW_SIZEY     16                  /* 状态文字 16 点阵 (等效 MP3 驱动 scale=2) */
#define MADCTL              0x36
#define MADCTL_VIDEO        0x70                /* 横屏 320x240, 同 lcd_init.c USE_HORIZONTAL==2 (无 BGR 位) */

static SemaphoreHandle_t s_draw_lock;
static uint16_t s_swap_buf[BLIT_CHUNK_PX];      /* 字节交换暂存 (受 s_draw_lock 保护) */

void LCD_BlitInit(void)
{
    if (s_draw_lock == NULL) {
        s_draw_lock = xSemaphoreCreateMutex();
    }
}

static void blit_lock(void)
{
    LCD_BlitInit();
    xSemaphoreTake(s_draw_lock, portMAX_DELAY);
}

static void blit_unlock(void)
{
    xSemaphoreGive(s_draw_lock);
}

/* 寄存器写 (lcd_init.c 的 LCD_WR_REG 是 static, 这里补一份) */
static void wr_reg(uint8_t cmd)
{
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    LCD_DC_Clr();
    spi_device_polling_transmit(spi, &t);
    LCD_DC_Set();
}

static void wr_data8(uint8_t dat)
{
    spi_transaction_t t = { .length = 8, .tx_buffer = &dat };
    spi_device_polling_transmit(spi, &t);
}

void LCD_EnterVideoMode(void)
{
    blit_lock();
    wr_reg(MADCTL);
    wr_data8(MADCTL_VIDEO);
    blit_unlock();
}

void LCD_WritePixels(int x, int y, int width, int height, const uint16_t *pixels)
{
    if (pixels == NULL || width <= 0 || height <= 0) {
        return;
    }
    /* 裁剪到视频模式逻辑屏内 */
    if (x < 0) { pixels -= x; width += x; x = 0; }
    if (y < 0) { pixels += (size_t)(-y) * width; height += y; y = 0; }
    if (x + width > LCD_WIDTH)  { width  = LCD_WIDTH - x; }
    if (y + height > LCD_HEIGHT) { height = LCD_HEIGHT - y; }
    if (width <= 0 || height <= 0) {
        return;
    }

    blit_lock();
    LCD_Address_Set((uint16_t)x, (uint16_t)y, (uint16_t)(x + width - 1), (uint16_t)(y + height - 1));
    LCD_DC_Set();
    const uint16_t *p = pixels;
    size_t remain = (size_t)width * height;
    unsigned chunk_n = 0;
    while (remain > 0) {
        size_t n = remain < BLIT_CHUNK_PX ? remain : BLIT_CHUNK_PX;
        for (size_t i = 0; i < n; i++) {
            s_swap_buf[i] = (uint16_t)((p[i] << 8) | (p[i] >> 8));   /* 大端发送序 */
        }
        spi_transaction_t t = { .length = n * sizeof(uint16_t) * 8, .tx_buffer = s_swap_buf };
        spi_device_polling_transmit(spi, &t);
        p += n;
        remain -= n;
        /* 整屏推流是上百毫秒 busy-wait, 周期放一拍给 IDLE, 防 task_wdt */
        if ((++chunk_n & 0x3F) == 0) {
            vTaskDelay(1);
        }
    }
    blit_unlock();
}

void LCD_FillRect(int x, int y, int width, int height, uint16_t color)
{
    if (width <= 0 || height <= 0) {
        return;
    }
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > LCD_WIDTH)  { width  = LCD_WIDTH - x; }
    if (y + height > LCD_HEIGHT) { height = LCD_HEIGHT - y; }
    if (width <= 0 || height <= 0) {
        return;
    }
    blit_lock();
    LCD_Fill((uint16_t)x, (uint16_t)y,
             (uint16_t)(x + width - 1), (uint16_t)(y + height - 1), color);   /* LCD_Fill 端点含 */
    blit_unlock();
}

void LCD_ShowText(int x, int y, const char *s, uint16_t fc, uint16_t bc)
{
    if (s == NULL) {
        return;
    }
    blit_lock();
    LCD_ShowString((uint16_t)x, (uint16_t)y, (const uint8_t *)s, fc, bc, BLIT_SHOW_SIZEY, 0);
    blit_unlock();
}
