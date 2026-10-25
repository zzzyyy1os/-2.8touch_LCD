/**
 * @file lvgl_port_touch.c
 * @brief LVGL 触摸 indev -> FT6236 (轮询)
 *
 * LVGL 默认 ~30ms 调一次 read_cb, 直接读 FT6236 寄存器足够。
 * 配网/LVGL 场景不要再启动 main/touch_input.c 的中断线程 (会重复初始化并抢 I2C)。
 * 坐标已是竖屏 240x320 直映 (ft6236.c 校准宏 REVERSE/TOUCH_MAX 与屏一致), 这里只做钳位。
 */
#include "esp_log.h"
#include "ft6236.h"
#include "lcd_init.h"
#include "lvgl_port.h"

static const char *TAG = "lvgl_port";

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    FT6236_Get_Touch_Data();

    if (FT6236_Instance.Touch_Count > 0) {
        int32_t x = FT6236_Instance.X_Pos;
        int32_t y = FT6236_Instance.Y_Pos;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > LCD_W - 1) x = LCD_W - 1;
        if (y > LCD_H - 1) y = LCD_H - 1;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t *lvgl_port_touch_create(void)
{
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    ESP_LOGI(TAG, "touch indev ready (FT6236 polling)");
    return indev;
}
