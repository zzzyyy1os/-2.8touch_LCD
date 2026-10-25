/**
 * @file wifi_provision.h
 * @brief SoftAP + 二维码 + Captive Portal WiFi 配网 (核心 API, 不依赖 LVGL)
 *
 * 流程: 无凭据 -> 开 SoftAP + DNS 劫持 + 配网页, 回调给出 WIFI:... 二维码内容,
 *       手机扫码连上热点 -> captive portal 弹配网页 -> 填 WiFi -> 存 NVS -> 重启连网。
 * 有凭据 -> STA 直连, 超时回落配网模式 (凭据保留, 按 Reset 才清除)。
 *
 * 前置: app 已调用 nvs_flash_init()。
 */
#ifndef WIFI_PROVISION_H
#define WIFI_PROVISION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_PROV_SSID_MAX          33
#define WIFI_PROV_PASS_MAX          65
#define WIFI_PROV_QR_PAYLOAD_MAX    128

typedef enum {
    WIFI_PROV_STATE_IDLE = 0,
    WIFI_PROV_STATE_CONNECTING,     /* 用已存凭据做 STA 连接 */
    WIFI_PROV_STATE_PROVISIONING,   /* AP + DNS + HTTP + QR 就绪 */
    WIFI_PROV_STATE_CONNECTED,      /* 已拿到 IP */
} wifi_provision_state_t;

typedef struct {
    const char *ap_ssid;            /* NULL -> "ESP32-Setup-XXXX" (MAC 后 2 字节) */
    const char *ap_pass;            /* NULL -> "12345678"; 空串 -> 开放网络 */
    int sta_connect_timeout_ms;     /* 0 -> 30000 */
    bool clear_creds_on_sta_fail;   /* false: STA 失败仅回落配网, 保留凭据 */

    /* 回调均可能来自内部任务, 更新 UI 需自行加 lvgl_port_lock */
    void (*on_state)(wifi_provision_state_t st);
    void (*on_provision_start)(const char *qr_payload); /* 进配网, 可直接喂 lv_qrcode */
    void (*on_sta_connecting)(const char *ssid);
    void (*on_got_ip)(const char *ip);
} wifi_provision_cfg_t;

/**
 * @brief 启动配网状态机 (内部自建 STA+AP netif 与 wifi 任务)
 * @note  cfg 内字符串会被拷贝, 调用方不需保活; 回调可为 NULL
 */
esp_err_t wifi_provision_start(const wifi_provision_cfg_t *cfg);

/** 清除 NVS 凭据并重启 (nvs_commit 完成后才 esp_restart) */
esp_err_t wifi_provision_reset(void);

/** 仅清凭据不重启 */
esp_err_t wifi_provision_clear_credentials(void);

bool wifi_provision_has_credentials(void);
wifi_provision_state_t wifi_provision_get_state(void);

/** 实际生效的热点名 (含 MAC 后缀), 供 UI 显示 */
const char *wifi_provision_get_ap_ssid(void);

/**
 * @brief 组装标准 WiFi QR 串: WIFI:S:<ssid>;T:<WPA|nopass>;P:<pass>;;
 *        ssid/pass 中的 \ ; , : " 按规范转义。需在 start() 之后调用。
 */
esp_err_t wifi_provision_get_qr_payload(char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROVISION_H */
