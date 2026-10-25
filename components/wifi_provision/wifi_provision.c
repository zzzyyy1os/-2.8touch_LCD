/**
 * @file wifi_provision.c
 * @brief 配网状态机: STA 直连 / 超时回落配网 / SoftAP+DNS+HTTP 就绪
 *
 * STA 失败保留凭据只回落配网 (路由器可能临时断电), 按 Reset 才清。
 * 用 APSTA 模式: /scan 扫描只在 STA 射频可用时工作, AP-only 会返回 WIFI_MODE 错误。
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "wifi_provision.h"
#include "wifi_provision_internal.h"

static const char *TAG = "wifi_prov";

#define GOT_IP_BIT      BIT0
#define RESET_BIT       BIT1    /* wifi_provision_reset 切回配网时唤醒 enter_sta 等待 */
#define DEFAULT_SSID_PREFIX "ESP32-Setup-"
#define DEFAULT_AP_PASS "12345678"
#define DEFAULT_TIMEOUT_MS  30000
#define STA_RETRY_MAX   5

static wifi_provision_cfg_t s_cfg;
static char s_cfg_ap_ssid[24];
static char s_cfg_ap_pass[WIFI_PROV_PASS_MAX];
static volatile wifi_provision_state_t s_state = WIFI_PROV_STATE_IDLE;
static EventGroupHandle_t s_events;
static char s_ssid[WIFI_PROV_SSID_MAX];
static char s_pass[WIFI_PROV_PASS_MAX];
static char s_ap_ssid[24];
static char s_ap_pass[WIFI_PROV_PASS_MAX];
static bool s_ap_open;
static char s_ip_str[16];
static bool s_handlers_registered;
static bool s_wifi_started;

static void enter_provision(void);
static void enter_sta(void);

static void wifi_start_once(void)
{
    if (!s_wifi_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
        s_wifi_started = true;
    }
}

/* ---------- WiFi 事件 ---------- */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    static int s_retry;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        s_retry = 0;
        if (s_state == WIFI_PROV_STATE_CONNECTING) {
            esp_wifi_connect();         /* 配网模式下 STA 空闲, 不许自动连 */
        }
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_state == WIFI_PROV_STATE_PROVISIONING) {
            return;                     /* 配网期间 STA 空闲, 无关 */
        }
        if (s_state == WIFI_PROV_STATE_CONNECTING) {
            if (s_retry < STA_RETRY_MAX) {
                s_retry++;
                ESP_LOGW(TAG, "STA connect failed, retry %d/%d", s_retry, STA_RETRY_MAX);
                esp_wifi_connect();
            }
            /* 超过重试计数后静默等待, 由 start() 的超时回落配网 */
        } else if (s_state == WIFI_PROV_STATE_CONNECTED) {
            esp_wifi_connect();         /* 已连过又断开: 自动重连 */
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&ev->ip_info.ip));
        ESP_LOGI(TAG, "got ip: %s", s_ip_str);
        xEventGroupSetBits(s_events, GOT_IP_BIT);
    }
}

/* ---------- 初始化容错 ---------- */
static esp_err_t net_stack_init(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    static esp_netif_t *s_sta_netif;
    static esp_netif_t *s_ap_netif;
    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (s_sta_netif == NULL || s_ap_netif == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init_cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;                     /* 双初始化等已就绪情形放行 */
    }

    if (!s_handlers_registered) {
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   wifi_event_handler, NULL));
        s_handlers_registered = true;
    }
    return ESP_OK;
}

/* ---------- QR / AP 参数 ---------- */
static void ap_params_build(void)
{
    if (s_cfg.ap_ssid && s_cfg.ap_ssid[0]) {
        strlcpy(s_ap_ssid, s_cfg.ap_ssid, sizeof(s_ap_ssid));
    } else {
        uint8_t mac[6] = {0};
        esp_wifi_get_mac(WIFI_IF_AP, mac);
        snprintf(s_ap_ssid, sizeof(s_ap_ssid), DEFAULT_SSID_PREFIX "%02X%02X",
                 mac[4], mac[5]);
    }

    if (s_cfg.ap_pass && s_cfg.ap_pass[0]) {
        strlcpy(s_ap_pass, s_cfg.ap_pass, sizeof(s_ap_pass));
        s_ap_open = false;
    } else if (s_cfg.ap_pass && s_cfg.ap_pass[0] == '\0') {
        s_ap_pass[0] = '\0';
        s_ap_open = true;
    } else {
        strlcpy(s_ap_pass, DEFAULT_AP_PASS, sizeof(s_ap_pass));
        s_ap_open = false;
    }
}

/* ---------- 进入配网模式 ---------- */
static void enter_provision(void)
{
    ap_params_build();

    /* 先置状态再 start: STA_START 事件里据此跳过自动连接 (竞态防护) */
    s_state = WIFI_PROV_STATE_PROVISIONING;

    /* APSTA: /scan 依赖 STA 射频 */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    /* 清 STA 配置并断开: 防止 STA 悄悄连上旧路由器把 SoftAP 拖到别的信道
     * (信道切换会让正在配网的手机掉线)。NVS 凭据不受影响。 */
    esp_wifi_disconnect();
    wifi_config_t sta_clear = {0};
    esp_wifi_set_config(WIFI_IF_STA, &sta_clear);

    wifi_config_t ap = {0};
    strlcpy((char *)ap.ap.ssid, s_ap_ssid, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen(s_ap_ssid);
    ap.ap.channel = 6;                  /* 固定信道, 扫描时 AP 不乱跳 */
    ap.ap.max_connection = 4;
    ap.ap.authmode = s_ap_open ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    if (!s_ap_open) {
        strlcpy((char *)ap.ap.password, s_ap_pass, sizeof(ap.ap.password));
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    esp_wifi_set_ps(WIFI_PS_NONE);      /* AP 期关省电, 提升 SoftAP 稳定性 */

    wifi_start_once();

    wifi_prov_dns_start();
    wifi_prov_http_start();

    char qr[WIFI_PROV_QR_PAYLOAD_MAX];
    if (wifi_prov_qr_build(qr, sizeof(qr), s_ap_ssid,
                           s_ap_open ? "" : s_ap_pass, s_ap_open) != ESP_OK) {
        strlcpy(qr, "WIFI:S:ESP32-Setup;T:nopass;;", sizeof(qr));
    }

    s_state = WIFI_PROV_STATE_PROVISIONING;
    ESP_LOGI(TAG, "provisioning mode: AP '%s' %s", s_ap_ssid,
             s_ap_open ? "(open)" : "(WPA2)");

    if (s_cfg.on_state) {
        s_cfg.on_state(WIFI_PROV_STATE_PROVISIONING);
    }
    if (s_cfg.on_provision_start) {
        s_cfg.on_provision_start(qr);
    }
}

/* ---------- 进入 STA 连接 ---------- */
static void enter_sta(void)
{
    s_state = WIFI_PROV_STATE_CONNECTING;
    if (s_cfg.on_state) {
        s_cfg.on_state(WIFI_PROV_STATE_CONNECTING);
    }
    if (s_cfg.on_sta_connecting) {
        s_cfg.on_sta_connecting(s_ssid);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t sta = {0};
    strlcpy((char *)sta.sta.ssid, s_ssid, sizeof(sta.sta.ssid));
    strlcpy((char *)sta.sta.password, s_pass, sizeof(sta.sta.password));
    sta.sta.threshold.authmode = WIFI_AUTH_OPEN;    /* 允许连开放网络 */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    wifi_start_once();
    esp_wifi_connect();

    int timeout = s_cfg.sta_connect_timeout_ms > 0
                      ? s_cfg.sta_connect_timeout_ms
                      : DEFAULT_TIMEOUT_MS;
    EventBits_t bits = xEventGroupWaitBits(s_events, GOT_IP_BIT | RESET_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(timeout));
    if ((bits & RESET_BIT) || s_state != WIFI_PROV_STATE_CONNECTING) {
        xEventGroupClearBits(s_events, RESET_BIT);
        return;                 /* 等待期间被 Reset 切回配网, 收尾已交给那边 */
    }
    if (bits & GOT_IP_BIT) {
        s_state = WIFI_PROV_STATE_CONNECTED;
        ESP_LOGI(TAG, "WiFi connected, ip=%s", s_ip_str);
        if (s_cfg.on_state) {
            s_cfg.on_state(WIFI_PROV_STATE_CONNECTED);
        }
        if (s_cfg.on_got_ip) {
            s_cfg.on_got_ip(s_ip_str);
        }
        return;
    }

    ESP_LOGW(TAG, "STA connect timeout (%d ms), fall back to provisioning", timeout);
    if (s_cfg.clear_creds_on_sta_fail) {
        wifi_prov_nvs_erase();
    }
    /* 停掉 STA 侧, 切配网 */
    xEventGroupClearBits(s_events, GOT_IP_BIT);
    enter_provision();
}

/* ---------- 公共 API ---------- */
esp_err_t wifi_provision_start(const wifi_provision_cfg_t *cfg)
{
    if (s_state != WIFI_PROV_STATE_IDLE) {
        return ESP_ERR_INVALID_STATE;
    }
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->ap_pass && cfg->ap_pass[0] && strlen(cfg->ap_pass) < 8) {
        return ESP_ERR_INVALID_ARG;     /* WPA2 密码至少 8 位 */
    }

    s_cfg = *cfg;                       /* 函数指针 + 标量拷贝 */
    if (cfg->ap_ssid) {
        strlcpy(s_cfg_ap_ssid, cfg->ap_ssid, sizeof(s_cfg_ap_ssid));
        s_cfg.ap_ssid = s_cfg_ap_ssid;
    }
    if (cfg->ap_pass) {
        strlcpy(s_cfg_ap_pass, cfg->ap_pass, sizeof(s_cfg_ap_pass));
        s_cfg.ap_pass = s_cfg_ap_pass;
    }
    memset(s_ssid, 0, sizeof(s_ssid));
    memset(s_pass, 0, sizeof(s_pass));

    s_events = xEventGroupCreate();
    if (s_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = net_stack_init();
    if (err != ESP_OK) {
        return err;
    }

    wifi_prov_nvs_load(s_ssid, sizeof(s_ssid), s_pass, sizeof(s_pass));

    if (s_ssid[0] != '\0') {
        ESP_LOGI(TAG, "saved credentials found for '%s', connecting", s_ssid);
        enter_sta();
    } else {
        enter_provision();
    }
    return ESP_OK;
}

esp_err_t wifi_provision_clear_credentials(void)
{
    return wifi_prov_nvs_erase();
}

esp_err_t wifi_provision_reset(void)
{
    esp_err_t err = wifi_prov_nvs_erase();
    if (err != ESP_OK) {
        return err;
    }
    memset(s_ssid, 0, sizeof(s_ssid));
    memset(s_pass, 0, sizeof(s_pass));

    if (s_events == NULL) {
        return ESP_ERR_INVALID_STATE;   /* 未 start, 只清凭据 */
    }
    xEventGroupClearBits(s_events, GOT_IP_BIT);
    xEventGroupSetBits(s_events, RESET_BIT);

    /* 无重启切回配网: enter_provision 里 on_provision_start 会重建配网页 */
    ESP_LOGW(TAG, "credentials cleared, back to provisioning (no restart)");
    enter_provision();
    return ESP_OK;
}

bool wifi_provision_has_credentials(void)
{
    return s_ssid[0] != '\0';
}

wifi_provision_state_t wifi_provision_get_state(void)
{
    return s_state;
}

const char *wifi_provision_get_ap_ssid(void)
{
    return s_ap_ssid;
}

esp_err_t wifi_provision_get_qr_payload(char *buf, size_t len)
{
    if (s_state != WIFI_PROV_STATE_PROVISIONING) {
        return ESP_ERR_INVALID_STATE;
    }
    return wifi_prov_qr_build(buf, len, s_ap_ssid,
                              s_ap_open ? "" : s_ap_pass, s_ap_open);
}

/* HTTP /connect 收到凭据后由 wifi_provision_http.c 调用: 通知 UI 进入连接中 */
void wifi_prov_core_notify_connecting(const char *ssid)
{
    s_state = WIFI_PROV_STATE_CONNECTING;
    if (s_cfg.on_state) {
        s_cfg.on_state(WIFI_PROV_STATE_CONNECTING);
    }
    if (s_cfg.on_sta_connecting) {
        s_cfg.on_sta_connecting(ssid);
    }
}
