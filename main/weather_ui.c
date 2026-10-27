/**
 * @file weather_ui.c
 * @brief 天气界面数据绑定实现
 *
 * 控件句柄来自 AiDoGUI 导出的 aidogui_ui.screen1.* (单例, 删除自动清空)。
 * 设计器导出的 ui_font___* 是按设计稿文字裁剪的子集字库, 动态文案
 * (任意城市名/天气词/数字) 会缺字, 所以动态控件换成 gen_fonts.py
 * 生成的宽字库 weather_font_*; 静态标题保持设计稿字体。
 */
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "esp_log.h"
#include "lvgl.h"
#include "ui.h"
#include "weather_fonts.h"
#include "weather_ui.h"

static const char *TAG = "weather_ui";

static lv_timer_t *s_clock_timer;

static void set_font(lv_obj_t *obj, const lv_font_t *font)
{
    if (obj && font) {
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
    }
}

/* ---------- 时钟/日期 (lv_timer 在 LVGL 线程跑, 无需加锁) ---------- */
static void clock_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    time_t now = time(NULL);
    if ((long long)now < WEATHER_TIME_VALID_EPOCH) {
        return;                     /* SNTP 未同步, 保持设计稿静态文字 */
    }
    struct tm tmv;
    localtime_r(&now, &tmv);

    /* 分钟没变就不动文本, 省掉每秒 3 个 label 的无谓重绘 */
    static struct tm s_last;
    static bool s_valid;
    if (s_valid && tmv.tm_min == s_last.tm_min && tmv.tm_hour == s_last.tm_hour &&
        tmv.tm_mday == s_last.tm_mday && tmv.tm_wday == s_last.tm_wday) {
        return;
    }
    s_last = tmv;
    s_valid = true;

    lv_obj_t *clock_l = aidogui_ui.screen1.clock_label;
    lv_obj_t *clock_r = aidogui_ui.screen1.clock_right;
    lv_obj_t *date_l = aidogui_ui.screen1.date_label;
    if (clock_l) {
        lv_label_set_text_fmt(clock_l, "%d", tmv.tm_hour);
    }
    if (clock_r) {
        lv_label_set_text_fmt(clock_r, ":%02d", tmv.tm_min);
    }
    if (date_l) {
        static const char *week[] = { "周日", "周一", "周二", "周三", "周四", "周五", "周六" };
        lv_label_set_text_fmt(date_l, "%s %d月%d日",
                              week[tmv.tm_wday % 7], tmv.tm_mon + 1, tmv.tm_mday);
    }
}

void weather_ui_init(void)
{
    /* 动态文本控件换宽字库 (城市名/天气词/任意数字用) */
    set_font(aidogui_ui.screen1.city_label,  &weather_font_30);
    set_font(aidogui_ui.screen1.date_label,  &weather_font_30);
    set_font(aidogui_ui.screen1.cond_label,  &weather_font_30);
    set_font(aidogui_ui.screen1.temp_label,  &weather_font_60);
    set_font(aidogui_ui.screen1.clock_label, &weather_font_60);
    set_font(aidogui_ui.screen1.clock_right, &weather_font_60);
    set_font(aidogui_ui.screen1.feels_label, &weather_font_16);
    set_font(aidogui_ui.screen1.hum_value,   &weather_font_16);
    set_font(aidogui_ui.screen1.temp_value,  &weather_font_16);
    set_font(aidogui_ui.screen1.range_label, &weather_font_16);

    /* 数据未到前清掉设计稿占位 (杭州/26°/多云 是设计器预览假数据),
     * 用 "--" 占位, 查询一成功 weather_ui_update 立即覆盖;
     * feels 标注"加载中...", 查询期不再是干等的静态屏 */
    if (aidogui_ui.screen1.city_label)  lv_label_set_text(aidogui_ui.screen1.city_label, "--");
    if (aidogui_ui.screen1.cond_label)  lv_label_set_text(aidogui_ui.screen1.cond_label, "--");
    if (aidogui_ui.screen1.temp_label)  lv_label_set_text(aidogui_ui.screen1.temp_label, "--°");
    if (aidogui_ui.screen1.range_label) lv_label_set_text(aidogui_ui.screen1.range_label, "--° / --°");
    if (aidogui_ui.screen1.feels_label) lv_label_set_text(aidogui_ui.screen1.feels_label, "加载中...");
    if (aidogui_ui.screen1.hum_value)   lv_label_set_text(aidogui_ui.screen1.hum_value, "--");
    if (aidogui_ui.screen1.temp_value)  lv_label_set_text(aidogui_ui.screen1.temp_value, "--°");
    if (aidogui_ui.screen1.hum_bar)     lv_bar_set_value(aidogui_ui.screen1.hum_bar, 0, LV_ANIM_OFF);
    if (aidogui_ui.screen1.temp_bar)    lv_bar_set_value(aidogui_ui.screen1.temp_bar, 0, LV_ANIM_OFF);

    if (s_clock_timer == NULL) {
        s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    }
    clock_timer_cb(s_clock_timer);      /* 立即刷新一次 */
    ESP_LOGI(TAG, "ui bound, clock timer up");
}

/* ---------- 天气数据 ---------- */
static int clamp_i(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

void weather_ui_update(const weather_data_t *d)
{
    if (d == NULL) {
        return;
    }

    if (aidogui_ui.screen1.city_label) {
        lv_label_set_text(aidogui_ui.screen1.city_label, d->city);
    }
    if (aidogui_ui.screen1.cond_label) {
        lv_label_set_text(aidogui_ui.screen1.cond_label, d->cond);
    }
    if (aidogui_ui.screen1.temp_label) {
        lv_label_set_text_fmt(aidogui_ui.screen1.temp_label, "%d°", d->temp);
    }
    if (aidogui_ui.screen1.range_label) {
        lv_label_set_text_fmt(aidogui_ui.screen1.range_label, "%d° / %d°", d->tmin, d->tmax);
    }

    /* 设计稿格式: "体感 27°   湿度 62   东南风 3级" (湿度后无百分号, 字库无 %) */
    if (aidogui_ui.screen1.feels_label) {
        lv_label_set_text_fmt(aidogui_ui.screen1.feels_label, "体感 %d°   湿度 %d   %s %s",
                              d->temp, d->hum,
                              d->wind_dir[0] ? d->wind_dir : "--",
                              d->wind_pow[0] ? d->wind_pow : "--");
    }

    if (aidogui_ui.screen1.hum_value) {
        lv_label_set_text_fmt(aidogui_ui.screen1.hum_value, "%d", d->hum);
    }
    if (aidogui_ui.screen1.hum_bar) {
        lv_bar_set_value(aidogui_ui.screen1.hum_bar, clamp_i(d->hum, 0, 100), LV_ANIM_OFF);
    }
    if (aidogui_ui.screen1.temp_value) {
        lv_label_set_text_fmt(aidogui_ui.screen1.temp_value, "%d°", d->temp);
    }
    if (aidogui_ui.screen1.temp_bar) {
        /* 量程 0-40 (设计稿), 负温钳到 0 */
        lv_bar_set_value(aidogui_ui.screen1.temp_bar, clamp_i(d->temp, 0, 40), LV_ANIM_OFF);
    }
}
