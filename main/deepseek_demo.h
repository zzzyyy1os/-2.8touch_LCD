/**
 * @file deepseek_demo.h
 * @brief DeepSeek 余额查询演示: 独立线程 + 屏幕显示 + 触摸刷新
 */
#ifndef DEEPSEEK_DEMO_H
#define DEEPSEEK_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动余额查询演示 (创建独立线程, 立即返回)
 *
 * 线程内: LCD 显示 → WiFi 连网 → 查询余额上屏;
 * 触摸屏轻点一下手动刷新, 并每 60s 自动刷新。
 */
void deepseek_demo_start(void);

#ifdef __cplusplus
}
#endif

#endif /* DEEPSEEK_DEMO_H */
