/**
 * @file provision_demo.c
 * @brief AP 配网 demo: 开机 LOGO 动画 + LVGL 界面 + SoftAP 二维码配网 + 天气界面
 *
 * 流程: 开机 LOGO 动画 (约 1.9s) 先上屏, 同时网络并行启动;
 * 无凭据 -> 屏显二维码热点配网页; 手机扫码连热点 -> captive portal 弹
 * 配网页填 WiFi -> 重启连网 -> 屏显 IP。有凭据 -> 直连(超时回落配网)。
 * 拿到 IP 后"连接成功"收尾屏短暂停留, 自动切入天气界面
 * (AiDoGUI 导出, components/aidogui, 数据接入接口在 custom/custom.c)。
 * 配网界面上 "Reset WiFi" 按钮清凭据重进配网。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "wifi_provision.h"
#include "wifi_provision_ui_lvgl.h"
#include "boot_anim.h"
#include "ui.h"
#include "weather_ui.h"
#include "weather_app.h"
#include "provision_demo.h"

static const char *TAG = "prov_demo";

/* 收尾屏(连接成功+IP)停留时长, 到点自动切天气界面 */
#define SWITCH_TO_WEATHER_MS    2500

static TimerHandle_t s_switch_timer;

static void boot_done(void)     /* esp_timer 上下文 (未持 lvgl 锁) */
{
    wifi_provision_ui_lvgl_set_hold(false);     /* 放行挂起的界面 */
}

/* 天气界面上屏 (软件定时器守护任务上下文, 未持 lvgl 锁) */
static void weather_takeover_cb(TimerHandle_t timer)
{
    (void)timer;
    /* 用户点了 Reset 重进配网就不切了; 等下次连上再触发 */
    if (wifi_provision_get_state() != WIFI_PROV_STATE_CONNECTED) {
        ESP_LOGW(TAG, "not connected anymore, skip weather takeover");
        return;
    }

    lv_obj_t *old = wifi_provision_ui_lvgl_release();   /* 接管界面控制权 */
    if (!lvgl_port_lock(2000)) {
        return;
    }
    aidogui_init();                         /* 建天气界面并上屏 */
    if (old && aidogui_ui.screen1.root) {
        lv_obj_delete(old);                 /* 先切新屏再删旧屏, 避免删活动屏 */
    }
    if (aidogui_ui.screen1.root) {
        weather_ui_init();                  /* 动态控件换宽字库 + 时钟定时器 */
        weather_app_flush_cache();          /* 收尾屏期间预取到的数据立即上屏 */
    }
    lvgl_port_unlock();
    ESP_LOGI(TAG, "weather UI up");
}

static void on_got_ip(const char *ip)
{
    ESP_LOGI(TAG, "got ip: %s, switching to weather UI in %d ms",
             ip ? ip : "?", SWITCH_TO_WEATHER_MS);
    /* 数据查询立即开跑, 与收尾屏 2.5s 并行, 界面上屏时可能已就绪 */
    weather_app_start();
    /* 本回调来自 wifi 任务, 延迟切屏交给软件定时器守护任务做 */
    xTimerReset(s_switch_timer, pdMS_TO_TICKS(100));
}

void provision_demo_start(void)
{
    ESP_LOGI(TAG, "AP provisioning demo (LVGL UI)");

    ESP_ERROR_CHECK(lvgl_port_init(NULL));

    s_switch_timer = xTimerCreate("wthr_sw", pdMS_TO_TICKS(SWITCH_TO_WEATHER_MS),
                                  pdFALSE, NULL, weather_takeover_cb);   /* 单次触发 */
    configASSERT(s_switch_timer != NULL);

    /* 开机动画先上屏 (非阻塞): 播放期间界面切换挂起, 网络照常并行启动 */
    wifi_provision_ui_lvgl_set_hold(true);
    boot_anim_start(boot_done);

    /* 会阻塞至连上 WiFi 或进入配网模式; 界面由 lvgl 任务并行刷新;
     * 连上后 on_got_ip -> 收尾屏短暂停留 -> 自动切天气界面 */
    static wifi_provision_cfg_t s_net_cfg = {
        .on_got_ip = on_got_ip,
    };
    static wifi_provision_ui_cfg_t s_ui_cfg = {
        .net_cfg = &s_net_cfg,
    };
    ESP_ERROR_CHECK(wifi_provision_ui_lvgl_start(&s_ui_cfg));
}
