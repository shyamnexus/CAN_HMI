/*
 * HMI Display Implementation
 * Creates and manages LVGL GUI for thermocouple data display
 */

#include "hmi_display.h"
#include "lvgl_port.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "HMI_DISPLAY";

// LVGL objects
static lv_obj_t *screen;
static lv_obj_t *title_label;
static lv_obj_t *channel_panels[MAX_THERMOCOUPLE_CHANNELS];
static lv_obj_t *temp_labels[MAX_THERMOCOUPLE_CHANNELS];
static lv_obj_t *status_labels[MAX_THERMOCOUPLE_CHANNELS];
static lv_obj_t *can_status_label;
static lv_obj_t *can_status_led;
static lv_obj_t *greeting_label;
static lv_timer_t *greeting_timer;

static TaskHandle_t hmi_update_task_handle = NULL;

// Color scheme
#define COLOR_BACKGROUND    lv_color_hex(0x1E1E1E)
#define COLOR_PANEL         lv_color_hex(0x2D2D2D)
#define COLOR_TITLE         lv_color_hex(0xFFFFFF)
#define COLOR_TEXT          lv_color_hex(0xE0E0E0)
#define COLOR_TEMP_NORMAL   lv_color_hex(0x4CAF50)
#define COLOR_TEMP_WARNING  lv_color_hex(0xFF9800)
#define COLOR_TEMP_DANGER   lv_color_hex(0xF44336)
#define COLOR_INVALID       lv_color_hex(0x757575)

static void greeting_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (greeting_label != NULL) {
        lv_obj_add_flag(greeting_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (greeting_timer != NULL) {
        lv_timer_pause(greeting_timer);
    }
}

static void greeting_button_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || greeting_label == NULL || greeting_timer == NULL) {
        return;
    }
    lv_label_set_text(greeting_label, "Hello Shyam");
    lv_obj_clear_flag(greeting_label, LV_OBJ_FLAG_HIDDEN);
    lv_timer_resume(greeting_timer);
    lv_timer_reset(greeting_timer);
}

/**
 * @brief Create temperature channel panel
 */
static lv_obj_t* create_channel_panel(lv_obj_t *parent, uint8_t channel, int x, int y) {
    // Create panel container
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 190, 100);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_style_bg_color(panel, COLOR_PANEL, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);

    // Channel number label
    lv_obj_t *ch_label = lv_label_create(panel);
    lv_obj_align(ch_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(ch_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(ch_label, &lv_font_montserrat_14, 0);
    char ch_text[16];
    snprintf(ch_text, sizeof(ch_text), "CH %d", channel);
    lv_label_set_text(ch_label, ch_text);

    // Temperature label
    lv_obj_t *temp_label = lv_label_create(panel);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -5);
    lv_obj_set_style_text_color(temp_label, COLOR_TEMP_NORMAL, 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(temp_label, "---°C");
    temp_labels[channel] = temp_label;

    // Status label
    lv_obj_t *status_label = lv_label_create(panel);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_color(status_label, COLOR_INVALID, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(status_label, "Initializing");
    status_labels[channel] = status_label;

    return panel;
}

esp_err_t hmi_display_init(void) {
    ESP_LOGI(TAG, "Initializing HMI display");

    // Lock LVGL mutex
    if (!lvgl_port_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return ESP_FAIL;
    }

    // Create main screen
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, COLOR_BACKGROUND, 0);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(screen);
    lv_obj_set_size(title_bar, LV_PCT(100), 50);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1976D2), 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);

    // Title label
    title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "K-Type Thermocouple CAN HMI");
    lv_obj_set_style_text_color(title_label, COLOR_TITLE, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 10, 0);

    // CAN status indicator
    can_status_led = lv_obj_create(title_bar);
    lv_obj_set_size(can_status_led, 20, 20);
    lv_obj_align(can_status_led, LV_ALIGN_RIGHT_MID, -60, 0);
    lv_obj_set_style_bg_color(can_status_led, lv_color_hex(0x757575), 0);
    lv_obj_set_style_radius(can_status_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(can_status_led, 0, 0);

    can_status_label = lv_label_create(title_bar);
    lv_label_set_text(can_status_label, "CAN");
    lv_obj_set_style_text_color(can_status_label, COLOR_TITLE, 0);
    lv_obj_set_style_text_font(can_status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(can_status_label, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_obj_t *greet_btn = lv_btn_create(title_bar);
    lv_obj_set_size(greet_btn, 90, 32);
    lv_obj_align(greet_btn, LV_ALIGN_RIGHT_MID, -170, 0);
    lv_obj_add_event_cb(greet_btn, greeting_button_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *greet_btn_label = lv_label_create(greet_btn);
    lv_label_set_text(greet_btn_label, "Greet");
    lv_obj_center(greet_btn_label);

    greeting_label = lv_label_create(screen);
    lv_label_set_text(greeting_label, "");
    lv_obj_set_style_text_color(greeting_label, COLOR_TITLE, 0);
    lv_obj_set_style_bg_color(greeting_label, lv_color_hex(0x374151), 0);
    lv_obj_set_style_bg_opa(greeting_label, LV_OPA_70, 0);
    lv_obj_set_style_radius(greeting_label, 10, 0);
    lv_obj_set_style_pad_all(greeting_label, 8, 0);
    lv_obj_align(greeting_label, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_flag(greeting_label, LV_OBJ_FLAG_HIDDEN);

    greeting_timer = lv_timer_create(greeting_timer_cb, 5000, NULL);
    lv_timer_pause(greeting_timer);

    // Create scrollable container for channels
    lv_obj_t *channel_container = lv_obj_create(screen);
    lv_obj_set_size(channel_container, LV_PCT(100), 400);
    lv_obj_set_pos(channel_container, 0, 90);
    lv_obj_set_style_bg_color(channel_container, COLOR_BACKGROUND, 0);
    lv_obj_set_style_border_width(channel_container, 0, 0);
    lv_obj_set_style_pad_all(channel_container, 5, 0);
    lv_obj_set_scrollbar_mode(channel_container, LV_SCROLLBAR_MODE_AUTO);

    // Create channel panels in 4 columns x 4 rows
    int panel_width = 190;
    int panel_height = 100;
    int spacing_x = 10;
    int spacing_y = 10;

    for (uint8_t i = 0; i < MAX_THERMOCOUPLE_CHANNELS; i++) {
        int col = i % 4;
        int row = i / 4;
        int x = col * (panel_width + spacing_x);
        int y = row * (panel_height + spacing_y);
        
        channel_panels[i] = create_channel_panel(channel_container, i, x, y);
    }

    // Load the screen
    lv_scr_load(screen);

    // Unlock LVGL mutex
    lvgl_port_unlock();

    ESP_LOGI(TAG, "HMI display initialized successfully");
    return ESP_OK;
}

void hmi_display_update_channel(uint8_t channel, thermocouple_data_t *data) {
    if (channel >= MAX_THERMOCOUPLE_CHANNELS || data == NULL) {
        return;
    }

    // Lock LVGL mutex
    if (!lvgl_port_lock(100)) {
        return;
    }

    // Update temperature label
    if (data->is_valid) {
        char temp_text[32];
        snprintf(temp_text, sizeof(temp_text), "%.1f°C", data->temperature);
        lv_label_set_text(temp_labels[channel], temp_text);

        // Set color based on temperature
        lv_color_t color;
        if (data->temperature < 0 || data->temperature > 150) {
            color = COLOR_TEMP_DANGER;
            lv_label_set_text(status_labels[channel], "Out of Range");
        } else if (data->temperature > 100) {
            color = COLOR_TEMP_WARNING;
            lv_label_set_text(status_labels[channel], "High Temp");
        } else {
            color = COLOR_TEMP_NORMAL;
            lv_label_set_text(status_labels[channel], "Normal");
        }
        lv_obj_set_style_text_color(temp_labels[channel], color, 0);
    } else {
        lv_label_set_text(temp_labels[channel], "---°C");
        lv_obj_set_style_text_color(temp_labels[channel], COLOR_INVALID, 0);
        lv_label_set_text(status_labels[channel], "Invalid");
    }

    // Unlock LVGL mutex
    lvgl_port_unlock();
}

void hmi_display_update_all(thermocouple_data_t *data, uint8_t num_channels) {
    if (data == NULL) {
        return;
    }

    for (uint8_t i = 0; i < num_channels && i < MAX_THERMOCOUPLE_CHANNELS; i++) {
        hmi_display_update_channel(i, &data[i]);
    }
}

void hmi_display_update_can_status(bool connected) {
    // Lock LVGL mutex
    if (!lvgl_port_lock(100)) {
        return;
    }

    if (connected) {
        lv_obj_set_style_bg_color(can_status_led, lv_color_hex(0x4CAF50), 0); // Green
    } else {
        lv_obj_set_style_bg_color(can_status_led, lv_color_hex(0xF44336), 0); // Red
    }

    // Unlock LVGL mutex
    lvgl_port_unlock();
}

/**
 * @brief HMI update task
 */
static void hmi_update_task(void *arg) {
    thermocouple_data_t temp_data[MAX_THERMOCOUPLE_CHANNELS];
    
    ESP_LOGI(TAG, "HMI update task started");

    while (1) {
        // Read all thermocouple channels
        if (thermocouple_read_all(temp_data, MAX_THERMOCOUPLE_CHANNELS) == ESP_OK) {
            // Update display
            hmi_display_update_all(temp_data, MAX_THERMOCOUPLE_CHANNELS);
            
            // Update CAN status (assume connected if no errors)
            hmi_display_update_can_status(true);
        } else {
            hmi_display_update_can_status(false);
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // Update at 2Hz
    }
}

esp_err_t hmi_display_start_update_task(void) {
    if (hmi_update_task_handle != NULL) {
        ESP_LOGW(TAG, "HMI update task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(hmi_update_task, "hmi_update", 4096, NULL, 4, &hmi_update_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HMI update task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "HMI update task created");
    return ESP_OK;
}
