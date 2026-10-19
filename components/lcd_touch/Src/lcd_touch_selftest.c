/* LCD + FT6236 触摸移植自检（验证通过后可整体移除本文件） */
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd.h"
#include "lcd_init.h"
#include "ft6236.h"
#include "lcd_touch_selftest.h"

static const char *TAG = "lcd_touch";

static void lcd_touch_selftest_task(void *arg)
{
    LCD_Init();
    FT6236_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    LCD_BLK_Set();

    ESP_LOGI(TAG, "LCD %dx%d init done", LCD_W, LCD_H);
    ESP_LOGI(TAG, "FT6236 chip id=0x%02x lib=0x%04x",
             FT6236_Get_ChipID(), FT6236_Get_LibVersion());

    /* 五色全屏刷屏 */
    const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, BLACK};
    for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
        LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, colors[i]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* 触摸画点循环：按下时在触点画圆并打印坐标 */
    ESP_LOGI(TAG, "touch test: draw with your finger");
    uint8_t was_touched = 0;
    while (1) {
        FT6236_Get_Touch_Data();
        if (FT6236_Instance.Touch_Count > 0) {
            uint16_t x = FT6236_Instance.X_Pos;
            uint16_t y = FT6236_Instance.Y_Pos;
            if (x >= LCD_W) x = LCD_W - 1;
            if (y >= LCD_H) y = LCD_H - 1;
            LCD_FillCircle(x, y, 3, YELLOW);
            if (!was_touched) {
                ESP_LOGI(TAG, "touch down (%u, %u)", x, y);
                was_touched = 1;
            }
        } else {
            was_touched = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void lcd_touch_selftest_start(void)
{
    /* 初始化与绘图较占栈，独立任务执行；优先级低于 espnow 任务 */
    xTaskCreate(lcd_touch_selftest_task, "lcd_touch_selftest", 4096, NULL, 2, NULL);
}
