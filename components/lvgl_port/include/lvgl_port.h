/**
 * @file lvgl_port.h
 * @brief LVGL 板级移植层 (2.8" ST7789V + FT6236, ESP32-S3)
 *
 * 一次初始化完成: LCD_Init + FT6236_Init + lv_init + 显示/触摸注册 + tick + lvgl 任务。
 * 跨线程操作 LVGL 前必须 lvgl_port_lock()/lvgl_port_unlock()。
 */
#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t hres;              /* 0 -> 320 (横屏) */
    uint16_t vres;              /* 0 -> 240 (横屏) */
    uint16_t buf_height;        /* partial buffer 行数, 0 -> 40 */
    uint32_t task_stack_size;   /* 0 -> 10240 */
    uint32_t task_priority;     /* 0 -> 5 */
} lvgl_port_cfg_t;

/**
 * @brief 初始化 LVGL 及板级外设 (LCD 横屏 320x240, FT6236 轮询 indev)
 *
 * 内部切 MADCTL=0x70 使 GRAM 为横屏 320x240; 触摸坐标已映射到同一空间。
 * @param cfg 可为 NULL 走默认值
 */
esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg);

lv_display_t *lvgl_port_get_display(void);
lv_indev_t *lvgl_port_get_indev(void);

/**
 * @brief 跨线程操作 LVGL 控件前加锁 (HTTP 任务/配网回调里改 UI 用)
 * @param timeout_ms 超时毫秒
 */
bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_PORT_H */
