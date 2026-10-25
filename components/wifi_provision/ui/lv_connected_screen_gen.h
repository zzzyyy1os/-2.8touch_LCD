/**
 * @file lv_connected_screen_gen.h
 * @brief Generated from lv_connected_screen.xml (LVGL Pro style)
 */

#ifndef LVGL_PRO_LV_CONNECTED_SCREEN_GEN_H
#define LVGL_PRO_LV_CONNECTED_SCREEN_GEN_H

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

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create the WiFi-connected status screen.
 * Named widgets for runtime access (lv_obj_find_by_name):
 *   "ssid" / "ip" / "btn_reset"
 */
lv_obj_t * lv_connected_screen_create(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LVGL_PRO_LV_CONNECTED_SCREEN_GEN_H */
