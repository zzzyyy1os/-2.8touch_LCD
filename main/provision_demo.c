/**
 * @file provision_demo.c
 * @brief AP 配网 demo: 开机 LOGO 动画 + LVGL 界面 + SoftAP 二维码配网
 *
 * 流程: 开机 LOGO 动画 (约 1.9s) 先上屏, 同时网络并行启动;
 * 无凭据 -> 屏显二维码热点配网页; 手机扫码连热点 -> captive portal 弹
 * 配网页填 WiFi -> 重启连网 -> 屏显 IP。有凭据 -> 直连(超时回落配网)。
 * 屏幕上 "Reset WiFi" 按钮清凭据重进配网。
 */
#include "esp_log.h"
#include "lvgl_port.h"
#include "wifi_provision_ui_lvgl.h"
#include "boot_anim.h"
#include "provision_demo.h"

static const char *TAG = "prov_demo";

static void boot_done(void)     /* esp_timer 上下文 (未持 lvgl 锁) */
{
    wifi_provision_ui_lvgl_set_hold(false);     /* 放行挂起的界面 */
}

void provision_demo_start(void)
{
    ESP_LOGI(TAG, "AP provisioning demo (LVGL UI)");

    ESP_ERROR_CHECK(lvgl_port_init(NULL));

    /* 开机动画先上屏 (非阻塞): 播放期间界面切换挂起, 网络照常并行启动 */
    wifi_provision_ui_lvgl_set_hold(true);
    boot_anim_start(boot_done);

    /* 会阻塞至连上 WiFi 或进入配网模式; 界面由 lvgl 任务并行刷新 */
    ESP_ERROR_CHECK(wifi_provision_ui_lvgl_start(NULL));
}
