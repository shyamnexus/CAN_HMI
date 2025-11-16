/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "waveshare_rgb_lcd_port.h"

// VSYNC event callback function
IRAM_ATTR static bool rgb_lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return lvgl_port_notify_rgb_vsync();
}

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
static uint8_t s_touch_dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;

static uint8_t detect_gt911_address(void)
{
    const uint8_t candidates[] = {
        ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP,
    };

    for (size_t i = 0; i < (sizeof(candidates) / sizeof(candidates[0])); i++) {
        if (board_i2c_probe_device(candidates[i]) == ESP_OK) {
            ESP_LOGI(TAG, "GT911 detected at 0x%02X", candidates[i]);
            return candidates[i];
        }
    }

    ESP_LOGW(TAG, "GT911 not detected on known addresses, falling back to 0x%02X", candidates[0]);
    return candidates[0];
}

// GPIO initialization
void gpio_init(void)
{
    // Zero-initialize the config structure
    gpio_config_t io_conf = {};
    // Disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    gpio_config(&io_conf);
}

// Reset the touch screen
void waveshare_esp32_s3_touch_reset()
{
    uint8_t write_buf = 0x01;
    board_i2c_write_byte(0x24, write_buf);

    // Reset the touch screen. It is recommended to reset the touch screen before using it.
    write_buf = 0x2C;
    board_i2c_write_byte(0x38, write_buf);
    esp_rom_delay_us(100 * 1000);
    gpio_set_level(GPIO_INPUT_IO_4, 0);
    esp_rom_delay_us(100 * 1000);
    write_buf = 0x2E;
    board_i2c_write_byte(0x38, write_buf);
    esp_rom_delay_us(200 * 1000);
}

#endif

// Initialize RGB LCD
esp_err_t waveshare_esp32_s3_rgb_lcd_init()
{
    ESP_LOGI(TAG, "Install RGB LCD panel driver"); // Log the start of the RGB LCD panel driver installation
    esp_lcd_panel_handle_t panel_handle = NULL; // Declare a handle for the LCD panel
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, // Set the clock source for the panel
        .timings =  {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ, // Pixel clock frequency
            .h_res = EXAMPLE_LCD_H_RES, // Horizontal resolution
            .v_res = EXAMPLE_LCD_V_RES, // Vertical resolution
            .hsync_pulse_width = 4, // Horizontal sync pulse width
            .hsync_back_porch = 8, // Horizontal back porch
            .hsync_front_porch = 8, // Horizontal front porch
            .vsync_pulse_width = 4, // Vertical sync pulse width
            .vsync_back_porch = 8, // Vertical back porch
            .vsync_front_porch = 8, // Vertical front porch
            .flags = {
                .pclk_active_neg = 1, // Active low pixel clock
            },
        },
        .data_width = EXAMPLE_RGB_DATA_WIDTH, // Data width for RGB
        .bits_per_pixel = EXAMPLE_RGB_BIT_PER_PIXEL, // Bits per pixel
        .num_fbs = LVGL_PORT_LCD_RGB_BUFFER_NUMS, // Number of frame buffers
        .bounce_buffer_size_px = EXAMPLE_RGB_BOUNCE_BUFFER_SIZE, // Bounce buffer size in pixels
        .sram_trans_align = 4, // SRAM transaction alignment
        .psram_trans_align = 64, // PSRAM transaction alignment
        .hsync_gpio_num = EXAMPLE_LCD_IO_RGB_HSYNC, // GPIO number for horizontal sync
        .vsync_gpio_num = EXAMPLE_LCD_IO_RGB_VSYNC, // GPIO number for vertical sync
        .de_gpio_num = EXAMPLE_LCD_IO_RGB_DE, // GPIO number for data enable
        .pclk_gpio_num = EXAMPLE_LCD_IO_RGB_PCLK, // GPIO number for pixel clock
        .disp_gpio_num = EXAMPLE_LCD_IO_RGB_DISP, // GPIO number for display
        .data_gpio_nums = {
            EXAMPLE_LCD_IO_RGB_DATA0,
            EXAMPLE_LCD_IO_RGB_DATA1,
            EXAMPLE_LCD_IO_RGB_DATA2,
            EXAMPLE_LCD_IO_RGB_DATA3,
            EXAMPLE_LCD_IO_RGB_DATA4,
            EXAMPLE_LCD_IO_RGB_DATA5,
            EXAMPLE_LCD_IO_RGB_DATA6,
            EXAMPLE_LCD_IO_RGB_DATA7,
            EXAMPLE_LCD_IO_RGB_DATA8,
            EXAMPLE_LCD_IO_RGB_DATA9,
            EXAMPLE_LCD_IO_RGB_DATA10,
            EXAMPLE_LCD_IO_RGB_DATA11,
            EXAMPLE_LCD_IO_RGB_DATA12,
            EXAMPLE_LCD_IO_RGB_DATA13,
            EXAMPLE_LCD_IO_RGB_DATA14,
            EXAMPLE_LCD_IO_RGB_DATA15,
        },
        .flags = {
            .fb_in_psram = 1, // Use PSRAM for framebuffer
        },
    };

    // Create a new RGB panel with the specified configuration
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel"); // Log the initialization of the RGB LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); // Initialize the LCD panel

    esp_lcd_touch_handle_t tp_handle = NULL; // Declare a handle for the touch panel
#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
    ESP_LOGI(TAG, "Initialize I2C bus"); // Log the initialization of the I2C bus
    ESP_ERROR_CHECK(board_i2c_init());
    i2c_master_bus_handle_t i2c_bus = NULL;
    ESP_ERROR_CHECK(board_i2c_get_bus(&i2c_bus));
    s_touch_dev_addr = detect_gt911_address();
    ESP_LOGI(TAG, "Initialize GPIO"); // Log GPIO initialization
    gpio_init(); // Initialize GPIO pins
    ESP_LOGI(TAG, "Initialize Touch LCD"); // Log touch LCD initialization
    waveshare_esp32_s3_touch_reset(); // Reset the touch panel

    esp_lcd_panel_io_handle_t tp_io_handle = NULL; // Declare a handle for touch panel I/O
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG(); // Configure I2C for GT911 touch controller
    tp_io_config.dev_addr = s_touch_dev_addr;
    tp_io_config.scl_speed_hz = I2C_MASTER_FREQ_HZ; // Synchronize with board I2C bus speed

    ESP_LOGI(TAG, "Initialize I2C panel IO"); // Log I2C panel I/O initialization
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &tp_io_handle)); // Create new I2C panel I/O

    ESP_LOGI(TAG, "Initialize touch controller GT911"); // Log touch controller initialization
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES, // Set maximum X coordinate
        .y_max = EXAMPLE_LCD_V_RES, // Set maximum Y coordinate
        .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST, // GPIO number for reset
        .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT, // GPIO number for interrupt
        .levels = {
            .reset = 0, // Reset level
            .interrupt = 0, // Interrupt level
        },
        .flags = {
            .swap_xy = 0, // No swap of X and Y
            .mirror_x = 0, // No mirroring of X
            .mirror_y = 0, // No mirroring of Y
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle)); // Create new I2C GT911 touch controller
#endif // CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911

    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle)); // Initialize LVGL with the panel and touch handles

    // Register callbacks for RGB panel events
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
#if EXAMPLE_RGB_BOUNCE_BUFFER_SIZE > 0
        .on_bounce_frame_finish = rgb_lcd_on_vsync_event, // Callback for bounce frame finish
#else
        .on_vsync = rgb_lcd_on_vsync_event, // Callback for vertical sync
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL)); // Register event callbacks

    return ESP_OK; // Return success 
}

/******************************* Turn on the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_on()
{
    //Configure CH422G to output mode 
    uint8_t write_buf = 0x01;
    board_i2c_write_byte(0x24, write_buf);

    //Pull the backlight pin high to light the screen backlight 
    write_buf = 0x1E;
    board_i2c_write_byte(0x38, write_buf);
    return ESP_OK;
}

/******************************* Turn off the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_off()
{
    //Configure CH422G to output mode 
    uint8_t write_buf = 0x01;
    board_i2c_write_byte(0x24, write_buf);

    //Turn off the screen backlight by pulling the backlight pin low 
    write_buf = 0x1A;
    board_i2c_write_byte(0x38, write_buf);
    return ESP_OK;
}

/******************************* Example code **************************************/
// Display a static study screen without continuous updates
void example_lvgl_demo_ui(void)
{
    lv_obj_t *scr = lv_scr_act();

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F141F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x05070C), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Thermocouple Study");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE5EAF5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "Channel Overview (static snapshot)");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 46);

    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_set_size(panel, 280, 170);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 30);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(panel, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *channel_a = lv_label_create(panel);
    lv_label_set_text(channel_a, "Channel A\n• Process Temp: 372.5 degC\n• Junction Temp: 41.2 degC\n• Status: Stable");
    lv_obj_set_style_text_color(channel_a, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *channel_b = lv_label_create(panel);
    lv_label_set_text(channel_b, "Channel B\n• Process Temp: 185.3 degC\n• Junction Temp: 32.8 degC\n• Status: Monitoring");
    lv_obj_set_style_text_color(channel_b, lv_color_hex(0xE2E8F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(channel_b, LV_ALIGN_TOP_LEFT, 0, 78);

    lv_obj_t *note = lv_label_create(panel);
    lv_label_set_text(note, "Notes\n• Study mode active\n• Screen will not auto-refresh");
    lv_obj_set_style_text_color(note, lv_color_hex(0xCBD5F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(note, LV_ALIGN_BOTTOM_LEFT, 0, -10);
}

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
esp_err_t waveshare_touch_recover(void)
{
    ESP_LOGW(TAG, "Attempting GT911/I2C bus recovery");
    esp_err_t ret = board_i2c_recover();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to recover I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }
    gpio_init();
    waveshare_esp32_s3_touch_reset();
    s_touch_dev_addr = detect_gt911_address();
    return ESP_OK;
}
#endif
