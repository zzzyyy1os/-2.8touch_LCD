/**
 * @file deep_app.h
 * @brief DeepSeek 余额数据任务 (FreeRTOS): 查询 -> 邮箱缓存 -> 上屏
 */
#ifndef DEEP_APP_H
#define DEEP_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/** 启动查询任务 (网络就绪后调用)。重复调用为空操作。 */
void deep_app_start(void);

/** "刷新余额"按钮: 任务通知唤醒查询, 无任务时为空操作 */
void deep_app_refresh_now(void);

/** 预取缓存立即上屏 (持 lvgl 锁调用, 无缓存则为空操作) */
void deep_app_flush_cache(void);

#ifdef __cplusplus
}
#endif

#endif /* DEEP_APP_H */
