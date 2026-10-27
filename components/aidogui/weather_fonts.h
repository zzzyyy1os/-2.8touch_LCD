/**
 * @file weather_fonts.h
 * @brief 动态文本宽字库 (gen_fonts.py 生成, 覆盖全国城市名/天气词/日期/数字)
 *
 * AiDoGUI 导出的 ui_font___* 是按设计稿文字裁剪的子集, 动态文案会缺字;
 * weather_ui.c 把动态控件的字体换成这三个。重新导出 UI 或换城市范围后,
 * 重跑 gen_fonts.py 重新生成 weather_font_*.c 即可。
 */
#ifndef WEATHER_FONTS_H
#define WEATHER_FONTS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(weather_font_16);
LV_FONT_DECLARE(weather_font_30);
LV_FONT_DECLARE(weather_font_60);

#ifdef __cplusplus
}
#endif

#endif /* WEATHER_FONTS_H */
