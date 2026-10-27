/**
 * @file weather_app.h
 * @brief 天气数据任务: IP 定位 -> 查天气 -> 刷 UI, 每小时自动刷新
 */
#ifndef WEATHER_APP_H
#define WEATHER_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动天气任务 (非阻塞)
 * @note  前置: WiFi 已连上。尽早调用 (收尾屏期间即可), 查询与界面切换并行;
 *        重复调用无效 (防 got-ip 重入)。SNTP 对时在这里一并启动。
 */
void weather_app_start(void);

/**
 * @brief 把预取缓存刷进天气界面
 * @note  天气界面建好 (weather_ui_init) 后、持 lvgl 锁时调用;
 *        查询还没出结果则无事发生, 稍后由任务自行刷 UI
 */
void weather_app_flush_cache(void);

#ifdef __cplusplus
}
#endif

#endif /* WEATHER_APP_H */
