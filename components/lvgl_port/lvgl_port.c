/**
 * @file lvgl_port.c
 * @brief LVGL 核心调度: tick / timer handler 任务 / 互斥锁
 *
 * tick 直接挂 esp_timer_get_time() (lv_tick_set_cb), 免 1ms 中断。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lcd.h"
#include "ft6236.h"
#include "lvgl_port.h"

static const char *TAG = "lvgl_port";

static SemaphoreHandle_t s_lvgl_mux;
static lv_display_t *s_disp;
static lv_indev_t *s_indev;

/* lvgl_port_disp.c / lvgl_port_touch.c */
lv_display_t *lvgl_port_disp_create(uint16_t hres, uint16_t vres, uint16_t buf_height);
lv_indev_t *lvgl_port_touch_create(void);

static uint32_t tick_get_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void lvgl_task(void *arg)
{
    for (;;) {
        if (lvgl_port_lock(20)) {
            uint32_t idle = lv_timer_handler();
            lvgl_port_unlock();
            if (idle > 20) {
                idle = 20;
            }
            vTaskDelay(pdMS_TO_TICKS(idle > 0 ? idle : 5));
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg)
{
    static lvgl_port_cfg_t def = {
        .hres = 240, .vres = 320, .buf_height = 40,
        .task_stack_size = 10240, .task_priority = 5,
    };
    if (cfg) {
        if (cfg->hres) def.hres = cfg->hres;
        if (cfg->vres) def.vres = cfg->vres;
        if (cfg->buf_height) def.buf_height = cfg->buf_height;
        if (cfg->task_stack_size) def.task_stack_size = cfg->task_stack_size;
        if (cfg->task_priority) def.task_priority = cfg->task_priority;
    }
    if (s_lvgl_mux) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 板级外设就绪 (竖屏 240x320, 全屏清白) */
    LCD_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, WHITE);
    LCD_BLK_Set();

    FT6236_Init();      /* 软 I2C + 复位 + 10ms 上报率 (勿再调 touch_input_start, 会抢 I2C) */

    lv_init();
    lv_tick_set_cb(tick_get_ms);

    s_disp = lvgl_port_disp_create(def.hres, def.vres, def.buf_height);
    if (s_disp == NULL) {
        return ESP_FAIL;
    }
    s_indev = lvgl_port_touch_create();
    if (s_indev == NULL) {
        return ESP_FAIL;
    }

    s_lvgl_mux = xSemaphoreCreateMutex();
    if (s_lvgl_mux == NULL) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(lvgl_task, "lvgl", def.task_stack_size, NULL,
                                def.task_priority, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "LVGL %d.%d.%d ready: %dx%d, buf %u lines",
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
             def.hres, def.vres, def.buf_height);
    return ESP_OK;
}

lv_display_t *lvgl_port_get_display(void)
{
    return s_disp;
}

lv_indev_t *lvgl_port_get_indev(void)
{
    return s_indev;
}

bool lvgl_port_lock(uint32_t timeout_ms)
{
    if (s_lvgl_mux == NULL) {
        return false;
    }
    TickType_t ticks = (timeout_ms == UINT32_MAX)
                           ? portMAX_DELAY
                           : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(s_lvgl_mux, ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    if (s_lvgl_mux) {
        xSemaphoreGive(s_lvgl_mux);
    }
}
