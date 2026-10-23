/**
 * @file deepseek_demo.c
 * @brief DeepSeek 余额查询演示实现
 *
 * 线程模型:
 *   ds_demo  — 独立工作线程: WiFi 连网 + HTTPS 查余额 + 刷屏 (TLS 吃栈, 开 16KB)
 *   touch    — touch_input 的中断驱动触摸线程, 轻点屏幕通知 ds_demo 立即刷新
 */
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lcd.h"
#include "lcd_init.h"
#include "deepseek.h"
#include "deepseek_config.h"
#include "touch_input.h"
#include "deepseek_demo.h"

static const char *TAG = "ds_demo";

#define WIFI_GOT_IP_BIT     BIT0
#define REFRESH_SEC         60

static EventGroupHandle_t s_wifi_events;
static TaskHandle_t s_demo_task;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();                     /* 断线自动重连 */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_events, WIFI_GOT_IP_BIT);
    }
}

static void wifi_connect(void)
{
    s_wifi_events = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t icfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&icfg);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wcfg = {0};
    strncpy((char *)wcfg.sta.ssid, WIFI_SSID, sizeof(wcfg.sta.ssid) - 1);
    strncpy((char *)wcfg.sta.password, WIFI_PASSWORD, sizeof(wcfg.sta.password) - 1);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi connecting to \"%s\" ...", WIFI_SSID);
    xEventGroupWaitBits(s_wifi_events, WIFI_GOT_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "wifi got ip");
}

/* 触摸线程回调: 轻点通知本演示线程刷新 (回调里不做耗时操作) */
static void on_touch(const FT6236_Info *info)
{
    if (info->Touch_Count > 0 && s_demo_task != NULL) {
        xTaskNotifyGive(s_demo_task);
    }
}

static void show_line(int y, const char *text, uint16_t color, uint8_t size)
{
    /* LCD_ShowString 不裁剪, 超长会画出屏外, 这里按字宽截断 */
    char buf[48];
    int max_chars = (LCD_W - 14) / (size / 2);
    if (max_chars > (int)sizeof(buf) - 1) {
        max_chars = sizeof(buf) - 1;
    }
    strlcpy(buf, text, max_chars + 1);

    LCD_Fill(10, y, LCD_W - 11, y + size + 4, BLACK);
    LCD_ShowString(12, y, (const uint8_t *)buf, color, BLACK, size, 0);
}

static void demo_task(void *arg)
{
    LCD_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    LCD_BLK_Set();
    LCD_ShowString(12, 16, (const uint8_t *)"DeepSeek Balance", WHITE, BLACK, 24, 0);
    LCD_ShowString(12, 56, (const uint8_t *)"tap screen to refresh", CYAN, BLACK, 16, 0);

    touch_input_start(on_touch);

    show_line(100, "WiFi connecting...", YELLOW, 16);
    wifi_connect();
    show_line(100, "WiFi OK", GREEN, 16);

    deepseek_init(DEEPSEEK_API_TOKEN);

    for (;;) {
        deepseek_balance_t bal;
        show_line(100, "querying...", YELLOW, 16);

        if (deepseek_get_balance(&bal) == ESP_OK) {
            char l1[40], l2[40];
            snprintf(l1, sizeof(l1), "%s %s",
                     bal.currency[0] ? bal.currency : "?",
                     bal.total_balance[0] ? bal.total_balance : "0.00");
            snprintf(l2, sizeof(l2), "available: %s  tap=refresh",
                     bal.is_available ? "YES" : "NO");
            show_line(100, l1, GREEN, 24);
            show_line(136, l2, YELLOW, 16);
            ESP_LOGI(TAG, "balance %s %s (granted %s, topped %s) available=%d",
                     bal.currency, bal.total_balance,
                     bal.granted_balance, bal.topped_up_balance, bal.is_available);
        } else {
            show_line(100, "query FAILED", RED, 24);
            show_line(136, deepseek_last_error(), YELLOW, 16);  /* 真实原因: net xxx / http status xxx */
        }

        /* 轻点立即刷新, 否则 60s 自动刷新 */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(REFRESH_SEC * 1000));
    }
}

void deepseek_demo_start(void)
{
    xTaskCreate(demo_task, "ds_demo", 16384, NULL, 5, &s_demo_task);
}
