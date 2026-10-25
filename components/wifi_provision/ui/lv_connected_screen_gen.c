/**
 * @file lv_connected_screen_gen.c
 * @brief Template source file for LVGL objects
 *
 * Generated from lv_connected_screen.xml; mirrors LVGL Pro export style.
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_connected_screen_gen.h"

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_connected_screen_create(void)
{
    lv_obj_t * root = lv_obj_create(NULL);
    lv_obj_set_name_static(root, "lv_connected_screen_#");
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(root, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(root, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_track_place(root, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_row(root, 12, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0xffffff), 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_label_create(root);
    lv_obj_set_name(title, "title");
    lv_label_set_text(title, "WiFi Connected");
    lv_obj_set_style_text_color(title, lv_color_hex(0x16a34a), 0);
#if LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
#endif

    lv_obj_t * ssid = lv_label_create(root);
    lv_obj_set_name(ssid, "ssid");
    lv_label_set_text(ssid, "SSID: --");
    lv_obj_set_style_text_color(ssid, lv_color_hex(0x374151), 0);

    lv_obj_t * ip = lv_label_create(root);
    lv_obj_set_name(ip, "ip");
    lv_label_set_text(ip, "IP: --");
    lv_obj_set_style_text_color(ip, lv_color_hex(0x374151), 0);

    lv_obj_t * btn_reset = lv_button_create(root);
    lv_obj_set_name(btn_reset, "btn_reset");
    lv_obj_set_size(btn_reset, 140, 40);

    lv_obj_t * btn_reset_label = lv_label_create(btn_reset);
    lv_obj_set_name(btn_reset_label, "btn_reset_label");
    lv_obj_align(btn_reset_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(btn_reset_label, "Reset WiFi");

    return root;
}
