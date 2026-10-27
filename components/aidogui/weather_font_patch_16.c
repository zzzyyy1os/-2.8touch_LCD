/*******************************************************************************
 * Size: 16 px
 * Bpp: 4
 * Opts: --font C:/Windows/Fonts/simhei.ttf --size 16 --bpp 4 --format lvgl --no-compress --symbols "%*-<>| --lv-font-name weather_font_patch_16 -o E:\esp\2.8touch\components\aidogui\weather_font_patch_16.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef WEATHER_FONT_PATCH_16
#define WEATHER_FONT_PATCH_16 1
#endif

#if WEATHER_FONT_PATCH_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0022 "\"" */
    0xf6, 0x7e, 0xe4, 0x6c, 0xd3, 0x5b, 0x61, 0x25,

    /* U+0025 "%" */
    0x1d, 0xc0, 0x1, 0x40, 0x78, 0xa5, 0x7, 0x20,
    0x96, 0x87, 0x9, 0x0, 0x96, 0x87, 0x44, 0x0,
    0x78, 0xa5, 0x90, 0x0, 0x1d, 0xc3, 0x70, 0x0,
    0x0, 0x8, 0x3d, 0xb0, 0x0, 0x8, 0x87, 0xb4,
    0x0, 0x63, 0xa5, 0x96, 0x0, 0x90, 0xa5, 0x96,
    0x3, 0x50, 0x87, 0xb4, 0x7, 0x0, 0x2d, 0xb0,

    /* U+002A "*" */
    0x0, 0x9, 0x90, 0x0, 0x12, 0x9, 0x90, 0x21,
    0x7f, 0x78, 0x87, 0xf7, 0x5, 0xdf, 0xfd, 0x50,
    0x1, 0x9f, 0xf9, 0x10, 0x5e, 0xca, 0xac, 0xe5,
    0x37, 0x8, 0x80, 0x73, 0x0, 0x9, 0x90, 0x0,
    0x0, 0x5, 0x50, 0x0,

    /* U+002D "-" */
    0xaf, 0xff, 0xff, 0xf6, 0x12, 0x22, 0x22, 0x20,

    /* U+003C "<" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0xd2,
    0x0, 0x0, 0x2d, 0x50, 0x0, 0x2, 0xd4, 0x0,
    0x0, 0x2d, 0x40, 0x0, 0x2, 0xd4, 0x0, 0x0,
    0x2d, 0x40, 0x0, 0x0, 0x1d, 0x50, 0x0, 0x0,
    0x1, 0xc6, 0x0, 0x0, 0x0, 0x1c, 0x60, 0x0,
    0x0, 0x0, 0xc7, 0x0, 0x0, 0x0, 0xc, 0x80,
    0x0, 0x0, 0x0, 0xb2, 0x0, 0x0, 0x0, 0x0,

    /* U+003E ">" */
    0x0, 0x0, 0x0, 0x0, 0x4c, 0x10, 0x0, 0x0,
    0x6, 0xc1, 0x0, 0x0, 0x0, 0x6c, 0x10, 0x0,
    0x0, 0x6, 0xc1, 0x0, 0x0, 0x0, 0x5c, 0x10,
    0x0, 0x0, 0x5, 0xd1, 0x0, 0x0, 0x7, 0xc0,
    0x0, 0x0, 0x7b, 0x0, 0x0, 0x8, 0xb0, 0x0,
    0x0, 0x9a, 0x0, 0x0, 0x9, 0xa0, 0x0, 0x0,
    0x4a, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    /* U+007C "|" */
    0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9,
    0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9,
    0xb9
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 8, .adv_w = 128, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 92, .adv_w = 128, .box_w = 8, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 100, .adv_w = 128, .box_w = 8, .box_h = 14, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 156, .adv_w = 128, .box_w = 8, .box_h = 14, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 212, .adv_w = 128, .box_w = 2, .box_h = 17, .ofs_x = 3, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x3, 0x8, 0xb, 0x1a, 0x1c, 0x5a
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 34, .range_length = 91, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 7, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t weather_font_patch_16 = {
#else
lv_font_t weather_font_patch_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if WEATHER_FONT_PATCH_16*/

