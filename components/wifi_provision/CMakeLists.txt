idf_component_register(
    SRCS
        "wifi_provision.c"
        "wifi_provision_nvs.c"
        "wifi_provision_dns.c"
        "wifi_provision_http.c"
        "wifi_provision_qr.c"
        "wifi_provision_ui_lvgl.c"
        "ui/lv_font_cn_20.c"
        "ui/lv_provisioning_screen_gen.c"
        "ui/lv_connected_screen_gen.c"
    INCLUDE_DIRS "include" "ui"
    REQUIRES esp_wifi esp_netif esp_event nvs_flash lvgl
    PRIV_REQUIRES esp_http_server lwip esp_timer esp_system freertos lvgl_port
)

# LVGL 头文件用 "lvgl.h" 简单包含 (lvgl 组件的 include 路径不含其父目录,
# "lvgl/lvgl.h" 形式解析不到)。UI 生成代码由 LVGL Pro 产出, 用宏切换包含方式。
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)

# LVGL Pro 生成代码用的 lv_obj_remove_flag 等在 LVGL 9.6 标了 deprecated, 仅告警不报错
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-declarations)
