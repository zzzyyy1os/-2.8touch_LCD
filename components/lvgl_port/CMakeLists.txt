idf_component_register(
    SRCS "lvgl_port.c" "lvgl_port_disp.c" "lvgl_port_touch.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl
    PRIV_REQUIRES lcd_touch driver esp_timer freertos heap
)
