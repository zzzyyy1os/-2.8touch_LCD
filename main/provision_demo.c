/**
 * @file provision_demo.c
 * @brief AP 配网 demo: LVGL 界面 + SoftAP 二维码配网
 *
 * 流程: 无凭据 -> 屏显二维码热点配网页; 手机扫码连热点 -> captive portal 弹
 * 配网页填 WiFi -> 重启连网 -> 屏显 IP。有凭据 -> 直连(超时回落配网)。
 * 屏幕上 "Reset WiFi" 按钮清凭据重进配网。
 */
#include "esp_log.h"
#include "lvgl_port.h"
#include "wifi_provision_ui_lvgl.h"
#include "provision_demo.h"

static const char *TAG = "prov_demo";

void provision_demo_start(void)
{
    ESP_LOGI(TAG, "AP provisioning demo (LVGL UI)");

    ESP_ERROR_CHECK(lvgl_port_init(NULL));

    /* 会阻塞至连上 WiFi 或进入配网模式; 界面由 lvgl 任务并行刷新 */
    ESP_ERROR_CHECK(wifi_provision_ui_lvgl_start(NULL));
}
