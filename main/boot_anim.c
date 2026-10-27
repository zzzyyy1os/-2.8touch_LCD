/**
 * @file boot_anim.c
 * @brief 开机 LOGO 动画
 *
 * 只用 opa/位置动画 (LVGL 9.6 软件渲染不支持缩放/旋转变换, 刻意避开)。
 * 结束回调经 FreeRTOS 软件定时器守护任务转接: anim 完成回调持着 lvgl 锁,
 * 交接逻辑 (set_hold 等) 需要在未持锁上下文执行, 避免互斥锁重入死锁。
 */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "boot_anim.h"
#include "boot_logo.h"

#define SCR_W           320
#define SCR_H           240
#define FADE_IN_MS      700
#define HOLD_MS         700
#define FADE_OUT_MS     450
#define SLIDE_IN_PX     16

static const char *TAG = "boot_anim";

static lv_obj_t *s_scr;
static void (*s_done)(void);
static TimerHandle_t s_release_timer;

/* ---------- anim exec 回调 ---------- */
static void set_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa(var, (lv_opa_t)v, 0);
}

static void set_y_cb(void *var, int32_t v)
{
    lv_obj_set_y(var, v);
}

static void release_cb(TimerHandle_t timer)
{
    (void)timer;
    if (s_done) {
        s_done();                       /* 界面交接 (未持 lvgl 锁) */
    }
    if (lvgl_port_lock(2000)) {
        /* 交接可能已加载新界面, 此时删动画屏; 仍是活动屏则留着 (兜底) */
        if (s_scr && lv_screen_active() != s_scr) {
            lv_obj_delete(s_scr);
            s_scr = NULL;
        } else {
            ESP_LOGW(TAG, "no screen took over, keeping logo on screen");
        }
        lvgl_port_unlock();
    }
}

static void fade_out_completed(lv_anim_t *a)
{
    (void)a;
    /* 换上下文跳板: 回调持着 lvgl 锁, 交接要回到未持锁任务上下文 */
    xTimerReset(s_release_timer, pdMS_TO_TICKS(100));
}

static void fade_in_completed(lv_anim_t *a)
{
    lv_obj_t *img = a->var;

    lv_anim_t fo;
    lv_anim_init(&fo);
    lv_anim_set_var(&fo, img);
    lv_anim_set_exec_cb(&fo, set_opa_cb);
    lv_anim_set_values(&fo, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&fo, FADE_OUT_MS);
    lv_anim_set_delay(&fo, HOLD_MS);
    lv_anim_set_completed_cb(&fo, fade_out_completed);
    lv_anim_start(&fo);
}

void boot_anim_start(void (*done)(void))
{
    s_done = done;

    if (s_release_timer == NULL) {
        /* 单次触发, 1 tick 后跑 release_cb, 仅作任务上下文跳板 */
        s_release_timer = xTimerCreate("boot_rel", pdMS_TO_TICKS(10),
                                       pdFALSE, NULL, release_cb);
        if (s_release_timer == NULL) {
            ESP_LOGE(TAG, "release timer create failed");
            return;
        }
    }

    if (!lvgl_port_lock(2000)) {
        return;
    }

    int32_t x = (SCR_W - boot_logo.header.w) / 2;
    int32_t y = (SCR_H - boot_logo.header.h) / 2;

    s_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_scrollable(s_scr, false);

    lv_obj_t *img = lv_image_create(s_scr);
    lv_image_set_src(img, &boot_logo);
    lv_obj_set_pos(img, x, y + SLIDE_IN_PX);        /* 起点: 偏下 */
    lv_obj_set_style_opa(img, LV_OPA_TRANSP, 0);

    lv_obj_t *old = lv_screen_active();
    lv_screen_load(s_scr);
    if (old && old != s_scr) {
        lv_obj_delete(old);
    }

    /* 淡入 + 上移到位 */
    lv_anim_t fi;
    lv_anim_init(&fi);
    lv_anim_set_var(&fi, img);
    lv_anim_set_exec_cb(&fi, set_opa_cb);
    lv_anim_set_values(&fi, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&fi, FADE_IN_MS);
    lv_anim_set_completed_cb(&fi, fade_in_completed);
    lv_anim_start(&fi);

    lv_anim_t mv;
    lv_anim_init(&mv);
    lv_anim_set_var(&mv, img);
    lv_anim_set_exec_cb(&mv, set_y_cb);
    lv_anim_set_values(&mv, y + SLIDE_IN_PX, y);
    lv_anim_set_duration(&mv, FADE_IN_MS);
    lv_anim_set_path_cb(&mv, lv_anim_path_ease_out);
    lv_anim_start(&mv);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "boot logo anim started (%dx%d)", boot_logo.header.w, boot_logo.header.h);
}
