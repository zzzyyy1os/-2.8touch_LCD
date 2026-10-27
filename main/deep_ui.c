/**
 * @file deep_ui.c
 * @brief DeepSeek 余额界面绑定 + 双屏半屏长按切换
 *
 * 触屏划左右两半: 长按左半边 -> 天气屏(左界面), 长按右半边 -> 余额屏(右界面)。
 * "刷新余额"按钮是例外区: 事件冒泡链在按钮处断开, 长按按钮/按钮文字都
 * 不会触发切屏; 短按按钮只刷新数据 (深通知唤醒查询任务)。
 *
 * 数据字段按 API 真实返回绑定:
 *   BalanceAmount  <- total_balance (账户余额)
 *   UpdatedLabel   <- 本机时间 "更新于 h:mm"
 *   StatusDot      <- is_available (绿/红)
 * (新版设计稿已去掉 可用/充值余额 两行与用量进度条;
 *  设计稿 ¥ 符号按要求删除, 余额行只留数字)
 */
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui.h"             /* 天气界面 (aidogui) */
#include "deepui.h"         /* 余额界面 (deepui) */
#include "deepseek.h"
#include "deep_ui.h"
#include "deep_app.h"       /* deep_app_refresh_now() */
#include "weather_ui.h"     /* WEATHER_TIME_VALID_EPOCH */

static const char *TAG = "deep_ui";

#define ZONE_MID_X      (SCREEN_WIDTH / 2)  /* 触屏左右两半分界 x=160 */
#define LONG_PRESS_MS   600                 /* 长按阈值, 防看屏时手搭屏误触 */

static bool s_inited;

/* 事件默认不冒泡, 逐个开 LV_OBJ_FLAG_EVENT_BUBBLE, 子控件上的长按
 * 都能到屏根回调。no_bubble 子树的冒泡链在此断开 (RefreshBtn 本身
 * 不开: 长按按钮/按钮文字都不会到屏根; 它的标签子对象照常冒泡到
 * 按钮, 短按文字仍走按钮刷新回调) */
static void event_bubble_deep(lv_obj_t *o, lv_obj_t *no_bubble)
{
    if (o != no_bubble) {
        lv_obj_add_flag(o, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
    uint32_t n = lv_obj_get_child_count(o);
    for (uint32_t i = 0; i < n; i++) {
        event_bubble_deep(lv_obj_get_child(o, i), no_bubble);
    }
}

/* 双屏共用: 长按左半 -> 天气屏(左界面), 长按右半 -> 余额屏(右界面) */
static void on_long_press(lv_event_t *e)
{
    lv_point_t p;
    lv_indev_get_point(lv_event_get_indev(e), &p);   /* 显示屏绝对坐标 */
    lv_obj_t *target = (p.x < ZONE_MID_X) ? aidogui_ui.screen1.root
                                          : deepui_ui.screen1.root;
    if (target && lv_screen_active() != target) {
        ESP_LOGI(TAG, "long-press x=%d -> %s", (int)p.x,
                 target == deepui_ui.screen1.root ? "deep" : "weather");
        /* 目标页从其一侧滑入 (左页天气从左进, 右页余额从右进), 两屏常驻 */
        lv_screen_load_anim(target,
                            target == deepui_ui.screen1.root ? LV_SCREEN_LOAD_ANIM_MOVE_LEFT
                                                             : LV_SCREEN_LOAD_ANIM_MOVE_RIGHT,
                            300, 0, false);
    }
}

/* "刷新余额"按钮短按: 只刷数据, 不切屏 (长按在按钮处即断开, 不触发切屏) */
static void on_refresh(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "refresh btn");
    deep_app_refresh_now();
}

void deep_ui_init(void)
{
    if (s_inited) {
        return;
    }
    deepui_init();                  /* 建屏并临时加载, 调用方决定最终停在哪屏 */
    if (deepui_ui.screen1.root == NULL) {
        ESP_LOGE(TAG, "deepui_init failed");
        return;
    }
    s_inited = true;

    /* 删掉设计稿的 ¥ 符号, 余额只留数字 (删除回调自动清句柄) */
    if (deepui_ui.screen1.BalanceSymbol) {
        lv_obj_delete(deepui_ui.screen1.BalanceSymbol);
    }

    /* 清设计稿占位假数据, 查询到真值前显示 "--" (字库补丁覆盖) */
    if (deepui_ui.screen1.BalanceAmount) {
        lv_label_set_text(deepui_ui.screen1.BalanceAmount, "--");
    }
    if (deepui_ui.screen1.UpdatedLabel) {
        lv_label_set_text(deepui_ui.screen1.UpdatedLabel, "更新于 --:--");
    }

    if (deepui_ui.screen1.RefreshBtn) {
        /* 短按才刷新 (长按+松开不发 SHORT_CLICKED, 不会误刷/误切) */
        lv_obj_add_event_cb(deepui_ui.screen1.RefreshBtn, on_refresh, LV_EVENT_SHORT_CLICKED, NULL);
    }

    /* 半屏长按切换: 阈值调到 600ms 防误触;
     * 子控件开事件气泡 (RefreshBtn 除外), 屏根收 LV_EVENT_LONG_PRESSED */
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_indev_set_long_press_time(indev, LONG_PRESS_MS);
        }
    }
    if (aidogui_ui.screen1.root) {
        event_bubble_deep(aidogui_ui.screen1.root, NULL);
        lv_obj_add_event_cb(aidogui_ui.screen1.root, on_long_press, LV_EVENT_LONG_PRESSED, NULL);
    }
    event_bubble_deep(deepui_ui.screen1.root, deepui_ui.screen1.RefreshBtn);
    lv_obj_add_event_cb(deepui_ui.screen1.root, on_long_press, LV_EVENT_LONG_PRESSED, NULL);
    ESP_LOGI(TAG, "deep UI bound, long-press L/R half to switch");
}

void deep_ui_update(const deepseek_balance_t *bal)
{
    if (bal == NULL || deepui_ui.screen1.root == NULL) {
        return;
    }
    char buf[48];

    if (deepui_ui.screen1.BalanceAmount) {
        lv_label_set_text(deepui_ui.screen1.BalanceAmount,
                          bal->total_balance[0] ? bal->total_balance : "--");
    }
    if (deepui_ui.screen1.UpdatedLabel) {
        time_t now = time(NULL);
        struct tm tm;
        if ((long long)now > WEATHER_TIME_VALID_EPOCH && localtime_r(&now, &tm)) {
            snprintf(buf, sizeof(buf), "更新于 %d:%02d", tm.tm_hour, tm.tm_min);
        } else {
            snprintf(buf, sizeof(buf), "更新于 --:--");   /* SNTP 未对时 */
        }
        lv_label_set_text(deepui_ui.screen1.UpdatedLabel, buf);
    }
    if (deepui_ui.screen1.StatusDot) {
        /* 绿 = 可继续调用, 红 = 余额不可用 */
        lv_led_set_color(deepui_ui.screen1.StatusDot, lv_color_hex(
                             bal->is_available ? 0x4FD39C : 0xF2555A));
    }
}
