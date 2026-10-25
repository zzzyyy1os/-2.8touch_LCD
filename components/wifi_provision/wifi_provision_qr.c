/**
 * @file wifi_provision_qr.c
 * @brief 标准 WiFi QR 内容组装 (WIFI:S:...;T:...;P:...;;)
 *
 * 手机扫码后自动连上 SoftAP, 无需手动选热点输密码。
 * 转义按 WiFi QR 约定: ssid/pass 内 \ ; , : " 前加反斜杠。
 */
#include <stdio.h>
#include <string.h>
#include "wifi_provision_internal.h"

static size_t qr_escape(char *dst, size_t dst_len, const char *src)
{
    size_t n = 0;
    for (; *src; src++) {
        if (strchr("\\;,:\"", *src)) {
            if (n + 1 >= dst_len) break;
            dst[n++] = '\\';
        }
        if (n + 1 >= dst_len) break;
        dst[n++] = *src;
    }
    dst[n] = '\0';
    return n;
}

esp_err_t wifi_prov_qr_build(char *buf, size_t len,
                             const char *ssid, const char *pass, bool is_open)
{
    if (buf == NULL || len == 0 || ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    char esc_ssid[WIFI_PROV_SSID_MAX * 2];
    char esc_pass[WIFI_PROV_PASS_MAX * 2];
    qr_escape(esc_ssid, sizeof(esc_ssid), ssid);

    int n;
    if (is_open || pass == NULL || pass[0] == '\0') {
        n = snprintf(buf, len, "WIFI:S:%s;T:nopass;;", esc_ssid);
    } else {
        qr_escape(esc_pass, sizeof(esc_pass), pass);
        /* T:WPA 覆盖 WPA/WPA2/WPA3, 各家扫码识别最稳 */
        n = snprintf(buf, len, "WIFI:S:%s;T:WPA;P:%s;;", esc_ssid, esc_pass);
    }
    return (n > 0 && (size_t)n < len) ? ESP_OK : ESP_ERR_INVALID_SIZE;
}
