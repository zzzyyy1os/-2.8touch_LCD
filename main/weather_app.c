/**
 * @file weather_app.c
 * @brief 天气数据任务实现
 *
 * 流程: SNTP 对时 (事件组信号) 与收尾屏并行启动;
 *       查询结果入长度 1 的队列邮箱, 天气界面建好后立即上屏;
 *       循环: 快路径按缓存城市查天气 -> 开机首轮 IP 定位校正城市
 *             (换城市补查一次) -> 刷 UI -> 睡 1 小时。
 *       查询失败清空城市名, 下轮重新定位 (城市失效/网络抖动自愈)。
 *       城市缓存存 NVS, 重启免定位直接出数据。
 */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "lvgl_port.h"
#include "weather.h"
#include "weather_ui.h"
#include "weather_config.h"
#include "weather_app.h"

static const char *TAG = "weather_app";

#define SNTP_SYNC_BIT   BIT0
#define CITY_NVS_NS     "weather"

static char s_city[24];         /* 当前定位到的城市 (为空表示待定位) */

/* SNTP 同步信号: sync_cb 置位, weather_task 等到才发 HTTPS (证书校验要真实时间) */
static EventGroupHandle_t s_sync_evt;

/* 查询结果缓存: 长度 1 队列当邮箱, xQueueOverwrite 只留最新一份
 * (FreeRTOS 自带线程安全, 不用自旋锁)。收尾屏 2.5s 期间预取,
 * 天气界面建好后立即上屏 */
static QueueHandle_t s_cache_q;

/** 天气界面建好后调用 (持 lvgl 锁): 预取已就绪则立即刷数据 */
void weather_app_flush_cache(void)
{
    weather_data_t d;
    if (s_cache_q && xQueuePeek(s_cache_q, &d, 0) == pdTRUE) {
        weather_ui_update(&d);
    }
}

/* ---------- 城市缓存 (NVS, 重启免定位) ---------- */
static void city_cache_load(void)
{
    nvs_handle_t h;
    if (nvs_open(CITY_NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return;                     /* 首次开机无缓存 */
    }
    size_t len = sizeof(s_city);
    if (nvs_get_str(h, "city", s_city, &len) != ESP_OK) {
        s_city[0] = '\0';
    }
    nvs_close(h);
}

static void city_cache_save(const char *city)
{
    nvs_handle_t h;
    if (nvs_open(CITY_NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    nvs_set_str(h, "city", city);
    nvs_commit(h);
    nvs_close(h);
}

/* ---------- SNTP 对时 ---------- */
static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    if (s_sync_evt) {
        xEventGroupSetBits(s_sync_evt, SNTP_SYNC_BIT);
    }
}

static void sntp_start(void)
{
    /* SNTP 同步的是 UTC, 不设 TZ 的话 localtime 会按 UTC 显示, 慢 8 小时 */
    setenv("TZ", "CST-8", 1);       /* 北京时间 (UTC+8, 无夏令时) */
    tzset();

    s_sync_evt = xEventGroupCreate();
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(WEATHER_NTP_SERVER);
    cfg.sync_cb = sntp_sync_cb;
    if (esp_netif_sntp_init(&cfg) == ESP_OK) {
        ESP_LOGI(TAG, "sntp started (%s)", WEATHER_NTP_SERVER);
    } else {
        ESP_LOGW(TAG, "sntp init failed");
    }
}

/* 查天气 -> 邮箱 + 刷 UI; 成功返回 true */
static bool query_and_publish(const char *city, weather_data_t *out)
{
    if (weather_query(city, out) != ESP_OK) {
        return false;
    }
    if (s_cache_q) {
        xQueueOverwrite(s_cache_q, out);
    }
    if (lvgl_port_lock(2000)) {
        weather_ui_update(out);
        lvgl_port_unlock();
    }
    return true;
}

static void weather_task(void *arg)
{
    (void)arg;
    weather_cfg_t cfg = {
        .ip_key = JUHE_IP_KEY,
        .weather_key = JUHE_WEATHER_KEY,
        .default_city = WEATHER_CITY_DEFAULT,
    };
    weather_init(&cfg);
    city_cache_load();
    if (s_city[0]) {
        ESP_LOGI(TAG, "cached city: %s", s_city);
    }

    /* 等 SNTP 首次对时完成再发 HTTPS: TLS 校验证书有效期要用真实系统时间,
     * 1970/2020 初值下证书"未生效", 握手全挂。SNTP 在任务创建前已启动,
     * 走到这通常瞬间返回 */
    EventBits_t bits = xEventGroupWaitBits(s_sync_evt, SNTP_SYNC_BIT,
                                          pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));
    if ((bits & SNTP_SYNC_BIT) == 0) {
        ESP_LOGW(TAG, "sntp not synced in 15s, https may fail this round");
    }

    bool first = true;
    for (;;) {
        weather_data_t data;
        bool just_located = false;

        /* 快路径: 缓存城市直接查 (一条 HTTPS, 早出数据) */
        if (s_city[0] != '\0' && query_and_publish(s_city, &data)) {
            /* 首轮后台定位校正: 设备搬家换城市则补查一次 (不挡首屏) */
            if (first) {
                char located[24];
                if (weather_locate(located, sizeof(located)) == ESP_OK &&
                    strcmp(located, s_city) != 0) {
                    ESP_LOGI(TAG, "city changed %s -> %s, re-query", s_city, located);
                    strlcpy(s_city, located, sizeof(s_city));
                    city_cache_save(s_city);
                    query_and_publish(s_city, &data);
                }
            }
        } else {
            s_city[0] = '\0';
            /* 定位失败时 weather_locate 也会填入兜底城市 */
            just_located = (weather_locate(s_city, sizeof(s_city)) == ESP_OK);
            if (just_located) {
                city_cache_save(s_city);    /* 只存 IP 真实定位的, 兜底城市不落盘 */
            }
            if (!query_and_publish(s_city, &data)) {
                s_city[0] = '\0';           /* 下轮重新定位 */
            }
        }
        first = false;

        vTaskDelay(pdMS_TO_TICKS(WEATHER_REFRESH_MIN * 60ULL * 1000ULL));
    }
}

void weather_app_start(void)
{
    static bool s_started;
    if (s_started) {
        return;                     /* Reset 重配网后再次 got-ip 不重复起任务 */
    }
    s_started = true;
    ESP_LOGI(TAG, "start, refresh every %d min", WEATHER_REFRESH_MIN);
    s_cache_q = xQueueCreate(1, sizeof(weather_data_t));    /* 邮箱: 只留最新一份 */
    sntp_start();                   /* 尽早对时: 与收尾屏/查询并行 */
    /* TLS 握手栈开销大 (对齐 deepseek_demo 的 16KB) */
    xTaskCreate(weather_task, "weather", 16384, NULL, 5, NULL);
}
