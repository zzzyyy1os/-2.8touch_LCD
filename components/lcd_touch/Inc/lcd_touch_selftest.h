#ifndef __LCD_TOUCH_SELFTEST_H
#define __LCD_TOUCH_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动 LCD + 触摸自检任务（移植验证用）
 *
 * 创建一个独立 FreeRTOS 任务，执行：
 *   1. LCD_Init() / FT6236_Init() 初始化
 *   2. 红绿蓝白黑五色全屏刷屏
 *   3. 进入触摸画点循环（按下画点，串口输出坐标）
 *
 * 初始化与循环都发生在该任务内，不占用 app_main 栈。
 * 验证完成后可删除调用点。
 */
void lcd_touch_selftest_start(void);

#ifdef __cplusplus
}
#endif

#endif
