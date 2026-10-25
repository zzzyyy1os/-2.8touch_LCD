/**
 * @file lv_provisioning_screen_gen.c
 * @brief Generated from lv_provisioning_screen.xml (LVGL Pro style)
 *
 * 横屏 320x240: 左二维码, 右列中文提示两行 + Reset 按钮。
 * hint/url 用自定义子集字体 lv_font_cn_20 (LVGL 默认字体无汉字)。
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_provisioning_screen_gen.h"

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_provisioning_screen_create(void)
{
    lv_obj_t * root = lv_obj_create(NULL);
    lv_obj_set_name_static(root, "lv_provisioning_screen_#");
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(root, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(root, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_track_place(root, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(root, 8, 0);
    lv_obj_set_style_pad_all(root, 6, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0xffffff), 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * qrcode = lv_qrcode_create(root);
    lv_obj_set_name(qrcode, "qrcode");
    lv_qrcode_set_size(qrcode, 160);
    lv_qrcode_set_dark_color(qrcode, lv_color_hex(0x111827));
    lv_qrcode_set_light_color(qrcode, lv_color_hex(0xffffff));
    lv_qrcode_set_data(qrcode, "WIFI:S:ESP32-Setup;T:WPA;P:12345678;;");
    lv_qrcode_set_quiet_zone(qrcode, true);
    lv_obj_set_style_border_color(qrcode, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_width(qrcode, 3, 0);

    /* 右列: 提示文字 + Reset (透明容器, 宽高随内容) */
    lv_obj_t * side = lv_obj_create(root);
    lv_obj_set_width(side, LV_SIZE_CONTENT);
    lv_obj_set_height(side, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(side, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(side, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(side, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_row(side, 8, 0);
    lv_obj_set_style_bg_opa(side, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(side, 0, 0);
    lv_obj_set_style_pad_all(side, 0, 0);
    lv_obj_remove_flag(side, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(side);
    lv_obj_set_name(hint, "hint");
    lv_label_set_text(hint, "WIFI链接网站");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x111827), 0);
    lv_obj_set_style_text_font(hint, &lv_font_cn_20, 0);

    lv_obj_t * url = lv_label_create(side);
    lv_obj_set_name(url, "url");
    lv_label_set_text(url, "192.168.4.1");
    lv_obj_set_style_text_color(url, lv_color_hex(0x111827), 0);
    lv_obj_set_style_text_font(url, &lv_font_cn_20, 0);

    lv_obj_t * btn_reset = lv_button_create(side);
    lv_obj_set_name(btn_reset, "btn_reset");
    lv_obj_set_size(btn_reset, 110, 34);

    lv_obj_t * btn_reset_label = lv_label_create(btn_reset);
    lv_obj_set_name(btn_reset_label, "btn_reset_label");
    lv_obj_align(btn_reset_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(btn_reset_label, "Reset WiFi");

    return root;
}
