/**
 * @file weather_ui.h
 * @brief 天气界面数据绑定 (AiDoGUI screen1 控件 -> weather_data_t / 时钟)
 *
 * 所有函数都要求在持 lvgl 锁的上下文调用 (内部不加锁);
 * 控件句柄随屏幕删除自动清空, 各更新函数内部判 NULL。
 */
#ifndef WEATHER_UI_H
#define WEATHER_UI_H

#include "weather.h"

/* 时间可信阈值 (2020-09-01): SNTP 对时前系统时间是 1970/2020 初值 */
#define WEATHER_TIME_VALID_EPOCH    1600000000LL

#ifdef __cplusplus
extern "C" {
#endif

/** 建屏后调用一次: 动态文本控件换宽字库字体 + 启动时钟定时器 */
void weather_ui_init(void);

/** 刷新天气数据 (城市/实况/温湿度卡片/今日温差) */
void weather_ui_update(const weather_data_t *d);

#ifdef __cplusplus
}
#endif

#endif /* WEATHER_UI_H */
