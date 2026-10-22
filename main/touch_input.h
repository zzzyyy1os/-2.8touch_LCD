/**
 * @file touch_input.h
 * @brief FT6236 触摸输入线程 (中断驱动, 无轮询)
 *
 * INT 引脚 (GPIO18) 下降沿唤醒独立触摸线程读取坐标并回调;
 * 无触摸事件时任务永久阻塞在任务通知上, 不占 CPU、不发 I2C。
 * 触摸芯片配置为触发模式 (G_MODE=0x01): 按下/移动按上报率重复
 * 触发中断, 抬手补一次 (Touch_Count=0)。
 */
#ifndef __TOUCH_INPUT_H__
#define __TOUCH_INPUT_H__

#include "ft6236.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 触摸事件回调 (运行在 touch 线程上下文, 勿做耗时操作)。
 * info->Touch_Count == 0 表示抬手 (此时坐标无效)。
 */
typedef void (*touch_input_cb_t)(const FT6236_Info *info);

/**
 * @brief 启动触摸输入: 初始化 FT6236 + INT 中断 + 独立线程
 * @param cb 事件回调, 可为 NULL (只内部记录)
 */
void touch_input_start(touch_input_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_INPUT_H__ */
