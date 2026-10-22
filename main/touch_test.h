/**
 * @file touch_test.h
 * @brief 触屏测试模式: 中断触摸线程 + 屏幕画点 + 串口坐标
 */
#ifndef __TOUCH_TEST_H__
#define __TOUCH_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动触屏测试: 点亮 LCD 显示标题, 手指按到哪画到哪,
 *        串口打印 press/release 坐标 (touch_input 独立线程)
 */
void touch_test_start(void);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_TEST_H__ */
