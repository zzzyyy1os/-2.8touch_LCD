#ifndef __LCD_TOUCH_SELFTEST_H
#define __LCD_TOUCH_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动 LCD 显示自检任务（点亮验证用）
 *
 * 创建一个独立 FreeRTOS 任务，执行：
 *   1. LCD_Init() 初始化 + 开背光
 *   2. 全屏循环刷 红/绿/蓝/白/黄 (每色 1s, 永不落黑)
 *
 * 屏幕持续变色即 LCD 通路正常 (SPI/复位/背光/初始化序列)。
 * 初始化与循环都发生在该任务内，不占用 app_main 栈。
 * 验证完成后可删除调用点。
 */
void lcd_touch_selftest_start(void);

#ifdef __cplusplus
}
#endif

#endif
