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
#include <stdlib.h>
#include <stdint.h>

static const char *TAG = "HMI_DISPLAY";
#define ADMIN_INPUT_COUNT 5

// LVGL objects
static lv_obj_t *main_screen;
static lv_obj_t *admin_screen;
static lv_obj_t *title_label;
static lv_obj_t *channel_panels[MAX_THERMOCOUPLE_CHANNELS];
static lv_obj_t *temp_labels[MAX_THERMOCOUPLE_CHANNELS];
static lv_obj_t *status_labels[MAX_THERMOCOUPLE_CHANNELS];
static lv_obj_t *can_status_label;
static lv_obj_t *can_status_led;
static lv_obj_t *admin_textareas[ADMIN_INPUT_COUNT];
static uint8_t admin_selected_index = 0;

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

typedef enum {
    ADMIN_KEY_UP = 0,
    ADMIN_KEY_DN,
    ADMIN_KEY_PLUS,
    ADMIN_KEY_MINUS,
    ADMIN_KEY_ENTER
} admin_key_t;

static void admin_button_event_cb(lv_event_t * e);
static void home_button_event_cb(lv_event_t * e);
static void admin_keyboard_event_cb(lv_event_t * e);
static void build_admin_screen(void);
static lv_obj_t *create_keyboard_button(lv_obj_t *parent, const char *text, admin_key_t key);
static void admin_select_input(uint8_t index);
static void admin_move_selection(int direction);
static void admin_adjust_value(int delta);
static void admin_enter_action(void);
static void show_main_screen(void);
static void show_admin_screen(void);

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
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, COLOR_BACKGROUND, 0);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(main_screen);
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

    // Admin button on title bar
    lv_obj_t *admin_button = lv_btn_create(title_bar);
    lv_obj_set_size(admin_button, 90, 32);
    lv_obj_align(admin_button, LV_ALIGN_RIGHT_MID, -210, 0);
    lv_obj_add_event_cb(admin_button, admin_button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *admin_btn_label = lv_label_create(admin_button);
    lv_label_set_text(admin_btn_label, "Admin");
    lv_obj_center(admin_btn_label);

    // Home button on title bar
    lv_obj_t *home_button = lv_btn_create(title_bar);
    lv_obj_set_size(home_button, 90, 32);
    lv_obj_align(home_button, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_add_event_cb(home_button, home_button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *home_btn_label = lv_label_create(home_button);
    lv_label_set_text(home_btn_label, "Home");
    lv_obj_center(home_btn_label);

    // Create scrollable container for channels
    lv_obj_t *channel_container = lv_obj_create(main_screen);
    lv_obj_set_size(channel_container, LV_PCT(100), 430);
    lv_obj_set_pos(channel_container, 0, 50);
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

    // Build admin screen
    build_admin_screen();

    // Load the main screen
    lv_scr_load(main_screen);

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
            //hmi_display_update_all(temp_data, MAX_THERMOCOUPLE_CHANNELS);
            
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

static void build_admin_screen(void) {
    admin_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(admin_screen, COLOR_BACKGROUND, 0);

    // Title bar
    lv_obj_t *title_bar = lv_obj_create(admin_screen);
    lv_obj_set_size(title_bar, LV_PCT(100), 50);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x455A64), 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);

    lv_obj_t *title = lv_label_create(title_bar);
    lv_label_set_text(title, "Admin");
    lv_obj_set_style_text_color(title, COLOR_TITLE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *home_button = lv_btn_create(title_bar);
    lv_obj_set_size(home_button, 90, 32);
    lv_obj_align(home_button, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(home_button, home_button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *home_label = lv_label_create(home_button);
    lv_label_set_text(home_label, "Home");
    lv_obj_center(home_label);

    // Inputs container
    lv_obj_t *inputs_container = lv_obj_create(admin_screen);
    lv_obj_set_size(inputs_container, 330, 380);
    lv_obj_set_pos(inputs_container, 20, 70);
    lv_obj_set_style_bg_opa(inputs_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(inputs_container, 0, 0);

    for (uint8_t i = 0; i < ADMIN_INPUT_COUNT; i++) {
        lv_obj_t *row = lv_obj_create(inputs_container);
        lv_obj_set_size(row, 300, 65);
        lv_obj_set_pos(row, 0, i * 75);
        lv_obj_set_style_bg_color(row, COLOR_PANEL, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x404040), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_all(row, 8, 0);

        char label_text[16];
        snprintf(label_text, sizeof(label_text), "Label %d", i + 1);

        lv_obj_t *label = lv_label_create(row);
        lv_label_set_text(label, label_text);
        lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *textarea = lv_textarea_create(row);
        lv_obj_set_size(textarea, 140, 40);
        lv_obj_align(textarea, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_textarea_set_one_line(textarea, true);
        lv_textarea_set_max_length(textarea, 6);
        lv_textarea_set_accepted_chars(textarea, "0123456789-");
        lv_textarea_set_text(textarea, "0");
        admin_textareas[i] = textarea;
    }

    admin_select_input(0);

    // Keyboard container
    lv_obj_t *keyboard_container = lv_obj_create(admin_screen);
    lv_obj_set_size(keyboard_container, 240, 280);
    lv_obj_set_pos(keyboard_container, 370, 120);
    lv_obj_set_style_bg_color(keyboard_container, COLOR_PANEL, 0);
    lv_obj_set_style_radius(keyboard_container, 8, 0);
    lv_obj_set_style_border_width(keyboard_container, 2, 0);
    lv_obj_set_style_border_color(keyboard_container, lv_color_hex(0x404040), 0);
    lv_obj_set_style_pad_all(keyboard_container, 10, 0);

    static lv_coord_t col_dsc[] = {100, 100, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(keyboard_container, col_dsc, row_dsc);

    lv_obj_t *btn_up = create_keyboard_button(keyboard_container, "Up", ADMIN_KEY_UP);
    lv_obj_set_grid_cell(btn_up, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t *btn_plus = create_keyboard_button(keyboard_container, "+", ADMIN_KEY_PLUS);
    lv_obj_set_grid_cell(btn_plus, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    lv_obj_t *btn_minus = create_keyboard_button(keyboard_container, "-", ADMIN_KEY_MINUS);
    lv_obj_set_grid_cell(btn_minus, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    lv_obj_t *btn_dn = create_keyboard_button(keyboard_container, "DN", ADMIN_KEY_DN);
    lv_obj_set_grid_cell(btn_dn, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);

    lv_obj_t *btn_enter = create_keyboard_button(keyboard_container, "Enter", ADMIN_KEY_ENTER);
    lv_obj_set_grid_cell(btn_enter, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 3, 1);
}

static lv_obj_t *create_keyboard_button(lv_obj_t *parent, const char *text, admin_key_t key) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_event_cb(btn, admin_keyboard_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)key);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

static void admin_button_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    show_admin_screen();
}

static void home_button_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    show_main_screen();
}

static void admin_keyboard_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    admin_key_t key = (admin_key_t)(intptr_t)lv_event_get_user_data(e);

    switch (key) {
        case ADMIN_KEY_UP:
            admin_move_selection(-1);
            break;
        case ADMIN_KEY_DN:
            admin_move_selection(1);
            break;
        case ADMIN_KEY_PLUS:
            admin_adjust_value(1);
            break;
        case ADMIN_KEY_MINUS:
            admin_adjust_value(-1);
            break;
        case ADMIN_KEY_ENTER:
            admin_enter_action();
            break;
        default:
            break;
    }
}

static void admin_select_input(uint8_t index) {
    if (index >= ADMIN_INPUT_COUNT) {
        return;
    }

    for (uint8_t i = 0; i < ADMIN_INPUT_COUNT; i++) {
        if (admin_textareas[i]) {
            lv_obj_clear_state(admin_textareas[i], LV_STATE_FOCUSED);
        }
    }

    if (admin_textareas[index]) {
        lv_obj_add_state(admin_textareas[index], LV_STATE_FOCUSED);
        admin_selected_index = index;
    }
}

static void admin_move_selection(int direction) {
    int new_index = (int)admin_selected_index + direction;

    if (new_index < 0) {
        new_index = ADMIN_INPUT_COUNT - 1;
    } else if (new_index >= ADMIN_INPUT_COUNT) {
        new_index = 0;
    }

    admin_select_input((uint8_t)new_index);
}

static void admin_adjust_value(int delta) {
    lv_obj_t *textarea = admin_textareas[admin_selected_index];
    if (!textarea) {
        return;
    }

    const char *text = lv_textarea_get_text(textarea);
    int value = 0;
    if (text && text[0] != '\0') {
        value = atoi(text);
    }

    value += delta;

    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", value);
    lv_textarea_set_text(textarea, buffer);
}

static void admin_enter_action(void) {
    lv_obj_t *textarea = admin_textareas[admin_selected_index];
    if (!textarea) {
        return;
    }

    const char *text = lv_textarea_get_text(textarea);
    ESP_LOGI(TAG, "Admin Enter pressed on Label %d -> %s", admin_selected_index + 1, text ? text : "");
}

static void show_main_screen(void) {
    if (main_screen) {
        lv_scr_load(main_screen);
    }
}

static void show_admin_screen(void) {
    if (admin_screen) {
        lv_scr_load(admin_screen);
    }
}
