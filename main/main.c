/* 2.8" 触摸 LCD 多功能固件入口 (ESP32-S3)
 *
 * 模式切换: 注释/取消注释 app_main 末尾对应的启动调用。
 * 触摸驱动 (components/lcd_touch)、DeepSeek 客户端 (components/deepseek) 见各自目录。
 *
 * ================ 引脚定义 (ESP32-S3) ================
 *
 * LCD ST7789V  240x320 RGB565, SPI2_HOST 10MHz        (components/lcd_touch/Inc/lcd_init.h)
 *   SCK  = 12      MOSI = 11     MISO = 未用
 *   CS   = 10      DC   = 9      RES  = 4
 *   BLK  = 5 (高电平点亮)
 *
 * 触摸 FT6236  软 I2C, 地址 0x38                      (components/lcd_touch/Inc/ft6236.h)
 *   SDA  = 15      SCL  = 16     (需外部上拉)
 *   RST  = 17      INT  = 18 (下降沿中断)
 *
 * SD 卡 (TF 模块)  SPI3_HOST                          (main/sdcard.h)
 *   MOSI = 13      MISO = 14     CLK  = 21     CS = 42
 *
 * 避让: Flash(26-32) / USB-JTAG(19,20) / UART0(43,44) /
 *       strapping(0,3,45,46) / PSRAM(35-37)
 * =====================================================
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "deepseek_demo.h"
#include "touch_test.h"
#include "video_app.h"
#include "lcd_touch_selftest.h"
#include "provision_demo.h"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ==== AP 配网模式: 屏显二维码, 手机扫码配网 (components/wifi_provision) ====
    provision_demo_start();
    // DeepSeek 余额查询: 换成 deepseek_demo_start(); (main/deepseek_config.h)
    // 触屏测试模式: 换成 touch_test_start();
    // 视频播放模式: 换成 video_app_start(); (SD 卡 /sd/test.mp4)
    // 显示屏自测模式: 换成 lcd_touch_selftest_start();
}
