# P024C135 LCD (ILI9341-compatible, SPI) + FT6236 capacitive touch driver
# Ported from vendor example P024C135 (ESP32-C3) to ESP32-S3.
idf_component_register(
    SRCS "Src/lcd_init.c" "Src/lcd.c" "Src/ft6236.c" "Src/iic_hal.c" "Src/lcd_touch_selftest.c"
    INCLUDE_DIRS "." "Inc"
    REQUIRES driver
    PRIV_REQUIRES esp_rom freertos
)
