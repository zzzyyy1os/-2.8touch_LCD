/**
 * @file touch_input.c
 * @brief FT6236 触摸输入线程实现 (INT 下降沿中断驱动)
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "touch_input.h"

static const char *TAG = "touch_input";

static TaskHandle_t s_touch_task;
static touch_input_cb_t s_cb;

static void IRAM_ATTR touch_isr(void *arg)
{
    BaseType_t hp = pdFALSE;
    vTaskNotifyGiveFromISR(s_touch_task, &hp);
    portYIELD_FROM_ISR(hp);
}

static void touch_task(void *arg)
{
    for (;;) {
        /* 无事件时在此永久阻塞 (任务通知, 非轮询) */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        FT6236_Get_Touch_Data();
        if (s_cb) {
            s_cb(&FT6236_Instance);
        }

        /* INT 仍为低且有触点: 数据未取尽/持续上报, 自唤醒继续处理;
         * 否则睡到下一个下降沿 */
        if (gpio_get_level(TOUCH_INT_PIN) == 0 && FT6236_Instance.Touch_Count > 0) {
            xTaskNotifyGive(s_touch_task);
        }
    }
}

void touch_input_start(touch_input_cb_t cb)
{
    s_cb = cb;

    FT6236_Init();      /* 软 I2C + 复位 + G_MODE=触发模式 + 上报率 10ms */

    ESP_LOGI(TAG, "FT6236 chip_id=0x%02X lib_ver=0x%04X",
             FT6236_Get_ChipID(), FT6236_Get_LibVersion());

    /* INT: 输入上拉, 下降沿中断 */
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << TOUCH_INT_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io);

    if (gpio_install_isr_service(0) != ESP_OK) {
        ESP_LOGW(TAG, "gpio isr service already installed");
    }
    gpio_isr_handler_add(TOUCH_INT_PIN, touch_isr, NULL);

    xTaskCreate(touch_task, "touch", 4096, NULL, 5, &s_touch_task);
    ESP_LOGI(TAG, "touch task started (INT gpio%d, interrupt-driven)", TOUCH_INT_PIN);
}
