# AiDoGUI 导出的 DEEPSEEK 余额界面 (LVGL 9, 320x240, 16bit 色深)
# port_deepui.py 从导出目录生成: 符号整体加 deepui_ 前缀与天气界面共存;
# 背景图像素共享 components/aidogui (同一素材, flash 只存一份);
# deepui_font_patch_* 是 simhei 补丁字库 (细笔画/生僻字缺失时 fallback),
# deepui_font_patch_*_x 是 arial 顶补丁 (simhei 没有的字如半角 ¥ 再回退)。
idf_component_register(
    SRCS
        "deepui.c"
        "deepui_helpers.c"
        "deepui_comm.c"
        "components/deepui_comp.c"
        "components/deepui_comp_hook.c"
        "screens/deepui_screen1.c"
        "events/deepui_event_screen1.c"
        "animations/deepui_animations.c"
        "fonts/deepui_fonts.c"
        "fonts/deepui_font___15.c"
        "fonts/deepui_font___18.c"
        "fonts/deepui_font___20.c"
        "fonts/deepui_font___30.c"
        "fonts/deepui_font___60.c"
        "fonts/deepui_font_patch_15.c"
        "fonts/deepui_font_patch_18.c"
        "fonts/deepui_font_patch_20.c"
        "fonts/deepui_font_patch_30.c"
        "fonts/deepui_font_patch_60.c"
        "fonts/deepui_font_patch_15_x.c"
        "fonts/deepui_font_patch_18_x.c"
        "fonts/deepui_font_patch_20_x.c"
        "fonts/deepui_font_patch_30_x.c"
        "fonts/deepui_font_patch_60_x.c"
        "custom/custom.c"
    INCLUDE_DIRS "." "custom"
    REQUIRES lvgl
)

# 生成代码用 "lvgl.h" 简单包含 (与 wifi_provision 组件保持一致)
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)

# 生成代码用的 lv_obj_remove_flag 等在 LVGL 9.6 标了 deprecated, 仅告警不报错
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-declarations)
