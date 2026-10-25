/**
 * @file wifi_provision_http.c
 * @brief 配网 HTTP 服务: 配网页 / 扫描 API / captive portal 探测劫持
 *
 * 路由:
 *   GET  /          配网页
 *   GET  /scan      周边 WiFi JSON (APSTA 下扫描)
 *   POST /connect   收凭据 -> 应答 -> 延时刷 TCP -> 存 NVS -> esp_restart
 *   探测路径 + 兜底  302 到 http://192.168.4.1/ (触发系统弹 captive 页)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "wifi_provision.h"
#include "wifi_provision_internal.h"
#include "web_page.h"

static const char *TAG = "prov_http";

#define AP_IP_URL       "http://192.168.4.1/"
#define BODY_MAX        256
#define SCAN_MAX_AP     20

static httpd_handle_t s_server;

/* 核心层: 通知 UI 进入"连接中"状态 (wifi_provision.c) */
void wifi_prov_core_notify_connecting(const char *ssid);

/* ---------- captive portal: 一律 302 到配网页 ---------- */
static esp_err_t captive_redirect_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", AP_IP_URL);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, NULL, 0);
}

/* ---------- GET / ---------- */
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, PAGE_INDEX, HTTPD_RESP_USE_STRLEN);
}

/* ---------- GET /scan ---------- */
static void json_escape(char *dst, size_t dst_len, const char *src)
{
    size_t n = 0;
    for (; *src && n + 2 < dst_len; src++) {
        if (*src == '"' || *src == '\\') {
            dst[n++] = '\\';
        }
        if ((unsigned char)*src < 0x20) {
            continue;                   /* 控制字符丢弃 */
        }
        dst[n++] = *src;
    }
    dst[n] = '\0';
}

static esp_err_t scan_handler(httpd_req_t *req)
{
    wifi_scan_config_t scfg = {
        .show_hidden = false,
        .scan_time.active = { .min = 40, .max = 120 },
    };
    esp_err_t err = esp_wifi_scan_start(&scfg, true);
    if (err != ESP_OK && err != ESP_ERR_WIFI_STATE) {
        ESP_LOGW(TAG, "scan_start failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "scan failed");
        return ESP_FAIL;
    }

    uint16_t ap_num = 0;
    esp_wifi_scan_get_ap_num(&ap_num);
    wifi_ap_record_t *ap = NULL;
    if (ap_num > 0) {
        ap = malloc(sizeof(wifi_ap_record_t) * ap_num);
    }
    if (ap) {
        esp_wifi_scan_get_ap_records(&ap_num, ap);
    } else {
        ap_num = 0;
    }

    /* 拼 JSON: [{"ssid":"..","rssi":-55,"auth":3},...] */
    char *out = malloc(2048);
    if (out == NULL) {
        free(ap);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
        return ESP_FAIL;
    }
    size_t pos = 0;
    pos += (size_t)snprintf(out + pos, 2048 - pos, "[");
    int shown = 0;
    for (int i = 0; i < ap_num && shown < SCAN_MAX_AP && pos < 1600; i++) {
        if (ap[i].ssid[0] == '\0') {
            continue;
        }
        char esc[80];
        json_escape(esc, sizeof(esc), (const char *)ap[i].ssid);
        pos += (size_t)snprintf(out + pos, 2048 - pos,
                                "%s{\"ssid\":\"%s\",\"rssi\":%d,\"auth\":%d}",
                                shown ? "," : "", esc, (int)ap[i].rssi,
                                (int)ap[i].authmode);
        shown++;
    }
    snprintf(out + pos, 2048 - pos, "]");
    free(ap);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t ret = httpd_resp_send(req, out, HTTPD_RESP_USE_STRLEN);
    free(out);
    return ret;
}

/* ---------- POST /connect ---------- */
static void url_decode(char *s)
{
    char *o = s;
    for (; *s; s++) {
        if (*s == '+') {
            *o++ = ' ';
        } else if (*s == '%' && s[1] && s[2]) {
            int hi = (s[1] >= 'A' ? (s[1] | 0x20) - 'a' + 10 : s[1] - '0');
            int lo = (s[2] >= 'A' ? (s[2] | 0x20) - 'a' + 10 : s[2] - '0');
            if (hi >= 0 && hi < 16 && lo >= 0 && lo < 16) {
                *o++ = (char)(hi * 16 + lo);
                s += 2;
            }
        } else {
            *o++ = *s;
        }
    }
    *o = '\0';
}

static bool form_get(const char *body, const char *key, char *val, size_t val_len)
{
    size_t klen = strlen(key);
    for (const char *p = body; p && *p; ) {
        /* 只匹配字段起点 (行首或 & 之后), 避免值里含 "key=" 时误命中 */
        if ((p == body || p[-1] == '&') &&
            strncmp(p, key, klen) == 0 && p[klen] == '=') {
            p += klen + 1;
            size_t n = 0;
            while (*p && *p != '&' && n + 1 < val_len) {
                val[n++] = *p++;
            }
            val[n] = '\0';
            url_decode(val);
            return true;
        }
        p = strchr(p, '&');
        if (p) {
            p++;
        }
    }
    return false;
}

static esp_err_t connect_handler(httpd_req_t *req)
{
    char body[BODY_MAX];
    int total = req->content_len;
    if (total <= 0 || total >= BODY_MAX) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
        return ESP_FAIL;
    }
    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, body + received, total - received);
        if (r <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "recv failed");
            return ESP_FAIL;
        }
        received += r;
    }
    body[received] = '\0';

    char ssid[WIFI_PROV_SSID_MAX] = {0};
    char pass[WIFI_PROV_PASS_MAX] = {0};
    bool ok = form_get(body, "ssid", ssid, sizeof(ssid));
    form_get(body, "pass", pass, sizeof(pass));

    /* 校验: SSID 非空 <=32; 密码空=开放网络, 非空则 WPA 要求 8..63 */
    if (!ok || ssid[0] == '\0' || strlen(ssid) > 32 ||
        (pass[0] != '\0' && (strlen(pass) < 8 || strlen(pass) > 63))) {
        ESP_LOGW(TAG, "connect rejected: bad ssid/pass");
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        return httpd_resp_send(req, PAGE_CONNECT_ERR, HTTPD_RESP_USE_STRLEN);
    }

    ESP_LOGI(TAG, "credentials received for ssid='%s', notifying UI", ssid);
    wifi_prov_core_notify_connecting(ssid);

    /* 顺序关键: 应答先发完 -> 延时让 TCP 刷出 -> 再存 NVS 重启, 否则手机一直转圈 */
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, PAGE_CONNECT_OK, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(800));

    esp_err_t err = wifi_prov_nvs_save(ssid, pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs save failed: %s", esp_err_to_name(err));
        return ESP_OK;                  /* 响应已发, 不再重启, 让用户重试 */
    }
    ESP_LOGI(TAG, "credentials saved, restarting...");
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_restart();
    return ESP_OK;                      /* 不可达 */
}

esp_err_t wifi_prov_http_start(void)
{
    if (s_server) {
        return ESP_ERR_INVALID_STATE;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.max_uri_handlers = 20;
    cfg.stack_size = 8192;
    cfg.lru_purge_enable = true;
    cfg.recv_wait_timeout = 5;
    cfg.send_wait_timeout = 5;

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        s_server = NULL;
        return err;
    }

    const httpd_uri_t uri_index = {
        .uri = "/", .method = HTTP_GET, .handler = index_handler,
    };
    const httpd_uri_t uri_scan = {
        .uri = "/scan", .method = HTTP_GET, .handler = scan_handler,
    };
    const httpd_uri_t uri_connect = {
        .uri = "/connect", .method = HTTP_POST, .handler = connect_handler,
    };
    httpd_register_uri_handler(s_server, &uri_index);
    httpd_register_uri_handler(s_server, &uri_scan);
    httpd_register_uri_handler(s_server, &uri_connect);

    /* 系统连通性探测路径 (Android/iOS/macOS/Windows/Firefox/Ubuntu),
     * 命中即 302 -> 系统弹 captive 浏览器 */
    static const char *const captive_paths[] = {
        "/generate_204", "/gen_204", "/mobile/status.php", "/check_network_status",
        "/hotspot-detect.html", "/library/test/success.html", "/success.html",
        "/connecttest.txt", "/ncsi.txt", "/redirect",
        "/canonical.html", "/success.txt", "/check_network_status.txt",
    };
    static httpd_uri_t captive_uri;
    captive_uri.method = HTTP_GET;
    captive_uri.handler = captive_redirect_handler;
    for (size_t i = 0; i < sizeof(captive_paths) / sizeof(captive_paths[0]); i++) {
        captive_uri.uri = captive_paths[i];
        httpd_register_uri_handler(s_server, &captive_uri);
    }

    /* 兜底: 其余 GET 一律 302 (注册必须放最后, 前缀匹配 *) */
    static httpd_uri_t catch_all;
    catch_all.uri = "/*";
    catch_all.method = HTTP_GET;
    catch_all.handler = captive_redirect_handler;
    httpd_register_uri_handler(s_server, &catch_all);

    ESP_LOGI(TAG, "provision http server started on :80");
    return ESP_OK;
}

void wifi_prov_http_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}
