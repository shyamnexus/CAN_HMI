idf_component_register(
    SRCS 
        "main.c"
        "thermocouple.c"
        "can_transmit.c"
        "hmi_display.c"
        "lvgl_port.c"
        "waveshare_rgb_lcd_port.c"
    INCLUDE_DIRS "."
)
