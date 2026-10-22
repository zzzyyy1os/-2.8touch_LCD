/* LCD + FT6236 触摸移植自检（验证通过后可整体移除本文件）
 * 当前为纯显示点亮测试: LCD_Init 后全屏循环刷红/绿/蓝/白/黄 (每色 1s),
 * 屏幕持续变色即 LCD 通路正常; 触摸画点测试暂移除, 需要时见 git 历史。 */
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd.h"
#include "lcd_init.h"
#include "lcd_touch_selftest.h"

static const char *TAG = "lcd_touch";

static void lcd_touch_selftest_task(void *arg)
{
    LCD_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    LCD_BLK_Set();

    ESP_LOGI(TAG, "LCD %dx%d init done, SPI=%d Hz, color loop start",
             LCD_W, LCD_H, LCD_SPI_FREQ_HZ);

    /* 五色全屏循环刷屏: 每色 1s, 永不落黑, 便于肉眼确认 */
    const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, YELLOW};
    const char *names[] = {"RED", "GREEN", "BLUE", "WHITE", "YELLOW"};
    const size_t n = sizeof(colors) / sizeof(colors[0]);
    size_t i = 0;
    while (1) {
        LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, colors[i]);
        ESP_LOGI(TAG, "fill %s", names[i]);
        i = (i + 1) % n;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void lcd_touch_selftest_start(void)
{
    /* 初始化与绘图较占栈，独立任务执行 */
    xTaskCreate(lcd_touch_selftest_task, "lcd_touch_selftest", 4096, NULL, 2, NULL);
}
