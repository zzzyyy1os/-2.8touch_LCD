/**
 * @file touch_test.c
 * @brief 触屏测试: 手指按到哪里, 就在哪里画彩色圆点
 *
 * 每次新按下换一种颜色 (拖动留下同色轨迹), 串口打印坐标;
 * 用于验证 触摸硬件 / 中断通路 / 坐标映射 (点应在手指正下方)。
 */
#include <stdint.h>
#include "esp_log.h"
#include "lcd.h"
#include "lcd_init.h"
#include "touch_input.h"
#include "touch_test.h"

static const char *TAG = "touch_test";

#define MARK_R          3   /* 画点半径 */

static const uint16_t s_colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE};
#define COLOR_N   (sizeof(s_colors) / sizeof(s_colors[0]))

static unsigned s_color_idx;
static uint8_t s_was_pressed;

/* 打点 (带边界裁剪, LCD_FillCircle 不裁剪会在边缘溢出) */
static void draw_mark(uint16_t x, uint16_t y, uint16_t color)
{
    int x1 = (int)x - MARK_R, y1 = (int)y - MARK_R;
    int x2 = (int)x + MARK_R, y2 = (int)y + MARK_R;
    if (x1 < 0) { x1 = 0; }
    if (y1 < 0) { y1 = 0; }
    if (x2 > LCD_W - 1) { x2 = LCD_W - 1; }
    if (y2 > LCD_H - 1) { y2 = LCD_H - 1; }
    LCD_Fill((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

static void on_touch(const FT6236_Info *info)
{
    if (info->Touch_Count == 0) {           /* 抬手 */
        if (s_was_pressed) {
            ESP_LOGI(TAG, "release");
            s_was_pressed = 0;
        }
        return;
    }

    if (!s_was_pressed) {                   /* 新一次按下: 换颜色 */
        s_color_idx++;
        ESP_LOGI(TAG, "press  x=%u y=%u  n=%d evt=%d",
                 info->X_Pos, info->Y_Pos, info->Touch_Count, info->Touch_Event);
        s_was_pressed = 1;
    }

    draw_mark(info->X_Pos, info->Y_Pos, s_colors[s_color_idx % COLOR_N]);
}

void touch_test_start(void)
{
    LCD_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    LCD_BLK_Set();

    LCD_ShowString(28, 16, (const uint8_t *)"TOUCH TEST", WHITE, BLACK, 24, 0);
    LCD_ShowString(28, 56, (const uint8_t *)"tap anywhere", CYAN, BLACK, 16, 0);

    touch_input_start(on_touch);
    ESP_LOGI(TAG, "touch test running (LCD %dx%d portrait)", LCD_W, LCD_H);
}
