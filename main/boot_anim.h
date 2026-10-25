/**
 * @file boot_anim.h
 * @brief 开机 LOGO 动画 (渐入上移 -> 停留 -> 渐出, 约 1.9s, 非阻塞)
 */
#ifndef BOOT_ANIM_H
#define BOOT_ANIM_H

/**
 * @brief 播放开机动画
 *
 * 黑底 LOGO 淡入并轻微上移, 停留后淡出; 约 1.9s 后回调 done。
 * 动画在 lvgl 任务里跑, 本函数立即返回, 不阻塞调用方 (网络可并行启动)。
 *
 * @param done 结束回调, 在 esp_timer 任务上下文执行 (未持 lvgl 锁),
 *             适合做界面交接 (如 wifi_provision_ui_lvgl_set_hold(false))。
 *             淡出后若已有新界面加载, 动画屏自动删除; 可为 NULL
 */
void boot_anim_start(void (*done)(void));

#endif /* BOOT_ANIM_H */
