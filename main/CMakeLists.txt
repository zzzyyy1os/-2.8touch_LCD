idf_component_register(SRCS "touch_test.c" "touch_input.c" "espnow_example_main.c" "video_app.c" "sdcard.c" "mp4play.c" "lcd_blit.c"
                    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi lcd_touch fatfs sdmmc esp_timer esp_video_codec esp_extractor
                    INCLUDE_DIRS ".")
