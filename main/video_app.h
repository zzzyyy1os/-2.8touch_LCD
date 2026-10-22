/**
 * @file video_app.h
 * @brief SD 卡视频播放启动入口 (可移植模块)
 *
 * 依赖模块: sdcard (SD 挂载), mp4play (解码播放), lcd_blit (推屏适配),
 * 以及 lcd_touch 组件的 LCD_Init (本模块负责调用)。
 *
 * 使用方法:
 *   app_main 里调用 video_app_start();   // 内部自建任务, 立即返回
 *
 * SD 卡根目录放 test.mp4 (ffmpeg 预转换 320x240 H.264, 见 mp4play.h),
 * 播放到结尾自动循环; 卡不在位/文件缺失时 LCD 报错并 3s 重试 (支持热插卡)。
 */
#ifndef __VIDEO_APP_H__
#define __VIDEO_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动视频播放 (LCD 初始化 + 背光 + SD 挂载 + 解码任务, 12K 栈, 核 1)
 */
void video_app_start(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIDEO_APP_H__ */
