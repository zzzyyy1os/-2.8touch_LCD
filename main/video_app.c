/**
 * @file video_app.c
 * @brief SD 卡视频播放启动入口 (LCD_Init + SD 挂载 + mp4play 任务)
 *
 * 初始化放在任务里做 (FATFS 挂载/解码栈深), app_main 立即返回;
 * espnow 等其他任务照常运行, 播放任务固定在核 1, LCD 推屏线程在核 0。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"
#include "lcd_init.h"
#include "sdcard.h"
#include "mp4play.h"
#include "lcd_blit.h"
#include "video_app.h"

#define MP4_TASK_STACK      (12 * 1024)
#define MP4_TASK_PRIO       5

static void mp4_decode_task(void *arg)
{
    LCD_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    vTaskDelay(pdMS_TO_TICKS(80));
    LCD_BLK_Set();                          /* 开背光 */
    LCD_BlitInit();
    SD_Init();
    MP4Play_LoopForever(SD_MOUNT_POINT "/test.mp4");   /* 阻塞, 永不返回 */
}

void video_app_start(void)
{
    xTaskCreatePinnedToCore(mp4_decode_task, "mp4_decode", MP4_TASK_STACK, NULL,
                            MP4_TASK_PRIO, NULL, 1);
}
