idf_component_register(SRCS "main.c" "touch_test.c" "touch_input.c" "deepseek_demo.c" "video_app.c" "sdcard.c" "mp4play.c" "lcd_blit.c" "provision_demo.c" "boot_anim.c" "boot_logo.c" "weather_ui.c" "weather_app.c"
                    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi lcd_touch fatfs sdmmc esp_timer esp_video_codec esp_extractor deepseek lvgl_port wifi_provision lvgl aidogui weather
                    INCLUDE_DIRS ".")
