# AiDoGUI 导出的天气界面 (LVGL 9, 320x240, 16bit 色深)
# 源文件列表对应导出目录的 filelist.txt; 背景图由 gen_bg_img.py 转换生成
# (原 ARGB8888 658x987 超出 flash 预算, 见 ui_img_*.c 头注释)。
idf_component_register(
    SRCS
        "ui.c"
        "ui_screen1.c"
        "ui_helpers.c"
        "ui_comm.c"
        "ui_comp.c"
        "ui_comp_hook.c"
        "ui_fonts.c"
        "ui_animations.c"
        "aido_event_screen1.c"
        "ui_font___16.c"
        "ui_font___30.c"
        "ui_font___60.c"
        "ui_img_t04f3a84e8f9ca6da16_jpg.c"
        "weather_font_16.c"
        "weather_font_30.c"
        "weather_font_60.c"
        "weather_font_patch_16.c"
        "weather_font_patch_30.c"
        "weather_font_patch_60.c"
        "custom/custom.c"
    INCLUDE_DIRS "." "custom"
    REQUIRES lvgl
)

# 生成代码用 "lvgl.h" 简单包含 (与 wifi_provision 组件保持一致)
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)

# 生成代码用的 lv_obj_remove_flag 等在 LVGL 9.6 标了 deprecated, 仅告警不报错
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-declarations)
