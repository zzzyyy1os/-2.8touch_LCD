/* 2.8" 触摸 LCD 多功能固件入口 (ESP32-S3)
 *
 * 模式切换: 注释/取消注释 app_main 末尾对应的启动调用。
 * 触摸驱动 (components/lcd_touch)、DeepSeek 客户端 (components/deepseek) 见各自目录。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "deepseek_demo.h"
#include "touch_test.h"
#include "video_app.h"
#include "lcd_touch_selftest.h"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ==== DeepSeek 余额查询: 独立线程 + WiFi + 触摸刷新 (main/deepseek_config.h) ====
    deepseek_demo_start();
    // 触屏测试模式: 换成 touch_test_start();
    // 视频播放模式: 换成 video_app_start(); (SD 卡 /sd/test.mp4)
    // 显示屏自测模式: 换成 lcd_touch_selftest_start();
}
