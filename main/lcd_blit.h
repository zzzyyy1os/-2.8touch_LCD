/**
 * @file lcd_blit.h
 * @brief LCD 视频推屏适配层 (可移植模块)
 *
 * 在 lcd_touch 组件 (lcd.h/lcd_init.h) 之上补齐视频播放需要的接口,
 * 并统一带绘制互斥锁 (lcd_task 推屏与解码线程状态文字可安全并发):
 *
 *   LCD_WritePixels  帧缓冲 RGB565 批量推屏 (内部字节序转换, 与 LCD_Fill 一致)
 *   LCD_FillRect     矩形填充 (转接 LCD_Fill, 宽高式坐标)
 *   LCD_ShowText     状态文字 (16 点阵 ASCII, 模拟 MP3 驱动 scale=2 效果)
 *   LCD_EnterVideoMode  切横屏 MADCTL, 逻辑屏 320x240 全屏放视频
 *
 * 视频模式下逻辑屏尺寸 LCD_WIDTH x LCD_HEIGHT = 320x240 (横屏);
 * 屏幕物理是 240x320 竖屏, 播放时把屏幕横过来看。
 * 上电默认仍是竖屏 (lcd_init.c USE_HORIZONTAL), 播放器启动时才切横屏。
 */
#ifndef __LCD_BLIT_H__
#define __LCD_BLIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lcd.h"
#include "lcd_init.h"

/* 视频模式 (横屏) 逻辑屏尺寸 */
#define LCD_WIDTH           320
#define LCD_HEIGHT          240

/* MP3 风格颜色别名 (lcd.h 裸宏的兼容名) */
#define LCD_BLACK           BLACK
#define LCD_WHITE           WHITE
#define LCD_RED             RED
#define LCD_GREEN           GREEN
#define LCD_BLUE            BLUE
#define LCD_CYAN            CYAN
#define LCD_YELLOW          YELLOW

/**
 * @brief 初始化绘制互斥锁 (video_app 启动时调用一次, 幂等)
 */
void LCD_BlitInit(void);

/**
 * @brief 切换横屏视频模式 (MADCTL=0x70, 320x240), 播放器启动时调用
 */
void LCD_EnterVideoMode(void);

/**
 * @brief RGB565 帧缓冲批量推屏 (整块连续 DMA 风格分片发送)
 *
 * @param x,y    左上角 (视频模式逻辑坐标, 自动裁剪到屏内)
 * @param width,height  区域尺寸
 * @param pixels RGB565 像素 (主机小端, 行宽 = width)
 */
void LCD_WritePixels(int x, int y, int width, int height, const uint16_t *pixels);

/**
 * @brief 矩形填充 (x,y 左上角 + 宽高, 自动裁剪到屏内)
 */
void LCD_FillRect(int x, int y, int width, int height, uint16_t color);

/**
 * @brief 画 16 点阵 ASCII 状态文字 (白底黑底可配, 无截断)
 */
void LCD_ShowText(int x, int y, const char *s, uint16_t fc, uint16_t bc);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_BLIT_H__ */
