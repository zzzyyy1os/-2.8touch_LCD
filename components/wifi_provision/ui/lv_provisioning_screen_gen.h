/**
 * @file lv_provisioning_screen_gen.h
 * @brief Generated from lv_provisioning_screen.xml (LVGL Pro style)
 */

#ifndef LVGL_PRO_LV_PROVISIONING_SCREEN_GEN_H
#define LVGL_PRO_LV_PROVISIONING_SCREEN_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#include "lv_font_cn_20.h"

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create the WiFi provisioning (QR) screen.
 * Named widgets for runtime access (lv_obj_find_by_name):
 *   "qrcode" / "ap_info" / "status" / "btn_reset"
 */
lv_obj_t * lv_provisioning_screen_create(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LVGL_PRO_LV_PROVISIONING_SCREEN_GEN_H */
