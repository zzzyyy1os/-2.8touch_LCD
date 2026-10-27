/**
 * @file deep_app.c
 * @brief DeepSeek 余额查询任务 (FreeRTOS)
 *
 *   长度 1 队列当结果邮箱 (xQueueOverwrite/xQueuePeek, 自带线程安全);
 *   "刷新余额"按钮走任务通知 (xTaskNotifyGive/ulTaskNotifyTake) 立即唤醒,
 *   否则 60s 自动刷新。时间未对时前不发请求 (TLS 证书校验要真时间)。
 */
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "lvgl_port.h"
#include "deepseek.h"
#include "deepseek_config.h"
#include "deep_ui.h"
#include "deep_app.h"
#include "weather_ui.h"     /* WEATHER_TIME_VALID_EPOCH */

static const char *TAG = "deep_app";

#define REFRESH_SEC     60
#define TIME_WAIT_MS    500     /* 等 SNTP 轮询步长 */
#define TIME_WAIT_MAX   30      /* 最多等 15s */

static TaskHandle_t s_task;
static QueueHandle_t s_cache_q;   /* 长度 1 队列当邮箱 */
static bool s_started;

void deep_app_flush_cache(void)
{
    deepseek_balance_t b;
    if (s_cache_q && xQueuePeek(s_cache_q, &b, 0) == pdTRUE) {
        deep_ui_update(&b);
    }
}

void deep_app_refresh_now(void)
{
    if (s_task) {
        xTaskNotifyGive(s_task);
    }
}

static void deep_task(void *arg)
{
    (void)arg;
    if (strstr(DEEPSEEK_API_TOKEN, "xxxx") != NULL) {
        ESP_LOGE(TAG, "DEEPSEEK_API_TOKEN is placeholder, fill main/deepseek_config.h");
    }
    deepseek_init(DEEPSEEK_API_TOKEN);

    /* 等 SNTP 对时完成 (weather_app_start 已启动 SNTP), 否则 TLS 校验挂 */
    for (int i = 0; i < TIME_WAIT_MAX; i++) {
        if ((long long)time(NULL) > WEATHER_TIME_VALID_EPOCH) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(TIME_WAIT_MS));
    }

    for (;;) {
        deepseek_balance_t b;
        if (deepseek_get_balance(&b) == ESP_OK) {
            ESP_LOGI(TAG, "balance %s (granted %s, topped %s) ok=%d",
                     b.total_balance, b.granted_balance, b.topped_up_balance, b.is_available);
            xQueueOverwrite(s_cache_q, &b);
            if (lvgl_port_lock(2000)) {
                deep_ui_update(&b);
                lvgl_port_unlock();
            }
        } else {
            ESP_LOGW(TAG, "query failed: %s", deepseek_last_error());
        }
        /* 60s 周期, 按钮任务通知提前唤醒 */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(REFRESH_SEC * 1000));
    }
}

void deep_app_start(void)
{
    if (s_started) {
        return;
    }
    s_started = true;
    s_cache_q = xQueueCreate(1, sizeof(deepseek_balance_t));
    configASSERT(s_cache_q != NULL);
    xTaskCreate(deep_task, "deep", 16384, NULL, 5, &s_task);
    configASSERT(s_task != NULL);
}
