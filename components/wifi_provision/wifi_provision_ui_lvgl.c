/**
 * @file wifi_provision_ui_lvgl.c
 * @brief 配网界面运行时接线 (本文件是 wifi_provision 组件中唯一依赖 LVGL 的实现文件)
 *
 * 界面构建在 ui/ 下的 *_gen.c (LVGL Pro 风格, 对应同名 *.xml)。
 * 配网回调可能来自 httpd 任务, 更新控件一律 lvgl_port_lock 包裹。
 */
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "wifi_provision.h"
#include "wifi_provision_ui_lvgl.h"
#include "lv_provisioning_screen_gen.h"
#include "lv_connected_screen_gen.h"

static const char *TAG = "prov_ui";

static wifi_provision_cfg_t s_net_cfg;      /* 链式接管后的实际配置 */
static const wifi_provision_cfg_t *s_user_cfg;
static lv_obj_t *s_scr;                     /* 当前界面 root */
static bool s_hold;                         /* 开机动画期间挂起界面切换 */
static lv_obj_t *s_pending;                 /* 挂起期间暂存的最新界面 */

/* ---------- 控件辅助 (均需持 lvgl 锁) ---------- */
static void screen_swap(lv_obj_t *scr)
{
    if (s_hold) {
        /* 只暂存不显示; 挂起期间多份时只留最新 */
        if (s_pending && s_pending != scr) {
            lv_obj_delete(s_pending);
        }
        s_pending = scr;
        return;
    }
    lv_obj_t *old = s_scr;
    s_scr = scr;
    lv_screen_load(s_scr);              /* 先切新屏再删旧屏, 避免删活动屏 */
    if (old) {
        lv_obj_delete(old);
    }
}

void wifi_provision_ui_lvgl_set_hold(bool hold)
{
    if (!lvgl_port_lock(2000)) {
        return;
    }
    s_hold = hold;
    if (!hold && s_pending) {
        lv_obj_t *p = s_pending;
        s_pending = NULL;
        screen_swap(p);                 /* 放行: 挂起期间攒的最新界面上屏 */
    }
    lvgl_port_unlock();
}

static esp_timer_handle_t s_reset_timer;

static void reset_defer_cb(void *arg)
{
    (void)arg;
    /* esp_timer 任务上下文 (未持 lvgl 锁): 清凭据并无重启切回配网,
     * enter_provision 触发 on_provision_start -> 重建配网页 */
    wifi_provision_reset();
}

static void reset_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGW(TAG, "reset requested, clearing credentials");
    /* 事件回调持着 lvgl 锁, 转 esp_timer 任务执行, 避免互斥锁重入 */
    if (s_reset_timer == NULL) {
        const esp_timer_create_args_t targs = {
            .callback = reset_defer_cb,
            .name = "prov_rst",
        };
        if (esp_timer_create(&targs, &s_reset_timer) != ESP_OK) {
            ESP_LOGE(TAG, "reset timer create failed");
            return;
        }
    }
    esp_timer_start_once(s_reset_timer, 0);
}

static void reset_btn_wire(lv_obj_t *root)
{
    lv_obj_t *btn = lv_obj_find_by_name(root, "btn_reset");
    if (btn) {
        lv_obj_add_event_cb(btn, reset_btn_event_cb, LV_EVENT_CLICKED, NULL);
    }
}

lv_obj_t *wifi_provision_ui_lvgl_release(void)
{
    lv_obj_t *taken = NULL;
    if (lvgl_port_lock(2000)) {
        taken = s_scr ? s_scr : s_pending;
        s_scr = NULL;
        s_pending = NULL;
        s_hold = false;
        lvgl_port_unlock();
    }
    return taken;
}

/* ---------- 界面切换 ---------- */
static void ui_show_provision(const char *qr_payload)
{
    if (!lvgl_port_lock(2000)) {
        return;
    }
    lv_obj_t *root = lv_provisioning_screen_create();
    screen_swap(root);

    lv_obj_t *qr = lv_obj_find_by_name(root, "qrcode");
    if (qr && qr_payload) {
        lv_qrcode_set_data(qr, qr_payload);
    }
    reset_btn_wire(root);
    lvgl_port_unlock();
}

void wifi_provision_ui_lvgl_show_connecting(const char *ssid)
{
    if (!lvgl_port_lock(2000)) {
        return;
    }
    lv_obj_t *root = lv_connected_screen_create();
    screen_swap(root);

    lv_obj_t *sl = lv_obj_find_by_name(root, "ssid");
    if (sl) {
        char buf[48];
        snprintf(buf, sizeof(buf), "SSID: %s", ssid ? ssid : "--");
        lv_label_set_text(sl, buf);
    }
    lv_obj_t *ip = lv_obj_find_by_name(root, "ip");
    if (ip) {
        lv_label_set_text(ip, "Connecting...");
    }
    reset_btn_wire(root);
    lvgl_port_unlock();
}

static void ui_show_connected(const char *ssid, const char *ip)
{
    if (!lvgl_port_lock(2000)) {
        return;
    }
    lv_obj_t *root = lv_connected_screen_create();
    screen_swap(root);

    lv_obj_t *sl = lv_obj_find_by_name(root, "ssid");
    if (sl) {
        char buf[48];
        snprintf(buf, sizeof(buf), "SSID: %s", ssid ? ssid : "--");
        lv_label_set_text(sl, buf);
    }
    lv_obj_t *ipw = lv_obj_find_by_name(root, "ip");
    if (ipw) {
        char buf[32];
        snprintf(buf, sizeof(buf), "IP: %s", ip ? ip : "--");
        lv_label_set_text(ipw, buf);
    }
    reset_btn_wire(root);
    lvgl_port_unlock();
}

/* ---------- 链式回调: 先更新界面, 再转发用户回调 ---------- */
static char s_cur_ssid[WIFI_PROV_SSID_MAX];

static void chain_on_provision_start(const char *qr_payload)
{
    ui_show_provision(qr_payload);
    if (s_user_cfg && s_user_cfg->on_provision_start) {
        s_user_cfg->on_provision_start(qr_payload);
    }
}

static void chain_on_sta_connecting(const char *ssid)
{
    strlcpy(s_cur_ssid, ssid ? ssid : "", sizeof(s_cur_ssid));
    wifi_provision_ui_lvgl_show_connecting(ssid);
    if (s_user_cfg && s_user_cfg->on_sta_connecting) {
        s_user_cfg->on_sta_connecting(ssid);
    }
}

static void chain_on_got_ip(const char *ip)
{
    ui_show_connected(s_cur_ssid, ip);
    if (s_user_cfg && s_user_cfg->on_got_ip) {
        s_user_cfg->on_got_ip(ip);
    }
}

static void chain_on_state(wifi_provision_state_t st)
{
    if (s_user_cfg && s_user_cfg->on_state) {
        s_user_cfg->on_state(st);
    }
}

esp_err_t wifi_provision_ui_lvgl_start(const wifi_provision_ui_cfg_t *ui_cfg)
{
    const wifi_provision_cfg_t *net =
        (ui_cfg && ui_cfg->net_cfg) ? ui_cfg->net_cfg : NULL;

    s_user_cfg = net;
    s_net_cfg = net ? *net : (wifi_provision_cfg_t){0};
    s_net_cfg.on_provision_start = chain_on_provision_start;
    s_net_cfg.on_sta_connecting = chain_on_sta_connecting;
    s_net_cfg.on_got_ip = chain_on_got_ip;
    s_net_cfg.on_state = chain_on_state;

    /* 有凭据时 start() 会先走 STA, 回调里切"连接中"界面 */
    ESP_LOGI(TAG, "starting provisioning UI + state machine");
    return wifi_provision_start(&s_net_cfg);
}
