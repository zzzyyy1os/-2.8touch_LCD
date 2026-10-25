/**
 * @file wifi_provision_ui_lvgl.h
 * @brief LVGL 配网界面 (QR 配网页 / 连接中 / 已连接 + Reset 按钮)
 *
 * 界面用 LVGL Pro 风格生成代码 (ui/ 下 lv_*_screen_gen.c, 由同名 *.xml 生成),
 * 本模块只做运行时接线: 回调更新控件 + Reset 按钮。
 * 前置条件: lvgl_port_init() 已完成。
 */
#ifndef WIFI_PROVISION_UI_LVGL_H
#define WIFI_PROVISION_UI_LVGL_H

#include "wifi_provision.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* 网络参数透传 (ap_ssid/ap_pass/回调等), NULL 走默认:
     * 回调会被链式接管: 先更新界面, 再转发给这里填的回调 */
    const wifi_provision_cfg_t *net_cfg;
} wifi_provision_ui_cfg_t;

/**
 * @brief 建界面并启动配网状态机 (内部调用 wifi_provision_start)
 * @note  会阻塞至 STA 连上或回落配网模式 (与 wifi_provision_start 一致),
 *        建议在独立任务或 app_main 末尾调用; 界面由 lvgl 任务并行刷新。
 */
esp_err_t wifi_provision_ui_lvgl_start(const wifi_provision_ui_cfg_t *ui_cfg);

/**
 * @brief 挂起/放行界面切换 (开机动画期间用)
 *
 * hold=true 期间, 回调触发的界面只建不显 (暂存), 动画结束调 hold=false
 * 一次性放行到最新界面。必须在未持 lvgl 锁的任务上下文调用 (内部自己加锁)。
 */
void wifi_provision_ui_lvgl_set_hold(bool hold);

/** 手动切"连接中"界面 (一般由 /connect 自动触发, 无需调用) */
void wifi_provision_ui_lvgl_show_connecting(const char *ssid);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROVISION_UI_LVGL_H */
