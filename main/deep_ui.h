/**
 * @file deep_ui.h
 * @brief DeepSeek 余额界面 (components/deepui) 绑定 + 双界面滑动切换
 *
 * deep_ui_init() 建 DEEP 余额屏、清设计稿占位假数据、挂刷新按钮回调,
 * 并注册半屏长按切换: 长按左半 -> 天气屏(左界面), 长按右半 -> 余额屏(右界面)。
 * "刷新余额"按钮区域除外 (长按按钮不切屏, 短按才刷新)。
 * 首屏由调用方决定 (provision_demo 切屏后 lv_screen_load 天气屏)。
 */
#ifndef DEEP_UI_H
#define DEEP_UI_H

#include "deepseek.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 建 DEEP 屏 + 绑定 + 手势注册。持 lvgl 锁调用; 重复调用为空操作。 */
void deep_ui_init(void);

/** 余额数据上屏 (持 lvgl 锁调用) */
void deep_ui_update(const deepseek_balance_t *bal);

#ifdef __cplusplus
}
#endif

#endif /* DEEP_UI_H */
