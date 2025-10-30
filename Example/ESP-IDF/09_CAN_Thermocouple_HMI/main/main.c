/*
 * CAN Thermocouple HMI Demo
 * 
 * This demo application combines:
 * - 16 channel K-type thermocouple reading (simulated)
 * - LVGL-based HMI display on 4.3" touch LCD
 * - CAN bus transmission of temperature data
 * 
 * Hardware: Waveshare ESP32-S3-Touch-LCD-4.3
 * Framework: ESP-IDF
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "waveshare_rgb_lcd_port.h"
#include "lvgl_port.h"
#include "thermocouple.h"
#include "can_transmit.h"
#include "hmi_display.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, " CAN Thermocouple HMI Demo");
    ESP_LOGI(TAG, " Waveshare ESP32-S3-Touch-LCD-4.3");
    ESP_LOGI(TAG, "===========================================");

    // Initialize NVS (required for WiFi and other features)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize LCD and LVGL
    ESP_LOGI(TAG, "Initializing LCD and LVGL...");
    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init());
    ESP_LOGI(TAG, "LCD and LVGL initialized successfully");

    // Initialize thermocouple interface
    ESP_LOGI(TAG, "Initializing thermocouple interface...");
    ESP_ERROR_CHECK(thermocouple_init());
    ESP_LOGI(TAG, "Thermocouple interface initialized");

    // Initialize CAN interface
    ESP_LOGI(TAG, "Initializing CAN interface...");
    ESP_ERROR_CHECK(can_init());
    ESP_LOGI(TAG, "CAN interface initialized");

    // Initialize HMI display
    ESP_LOGI(TAG, "Creating HMI interface...");
    ESP_ERROR_CHECK(hmi_display_init());
    ESP_LOGI(TAG, "HMI interface created");

    // Start background tasks
    ESP_LOGI(TAG, "Starting background tasks...");
    
    // Start HMI update task
    ESP_ERROR_CHECK(hmi_display_start_update_task());
    
    // Start CAN transmission task
    ESP_ERROR_CHECK(can_start_transmission_task());

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, " System initialized successfully!");
    ESP_LOGI(TAG, " - 16 thermocouple channels active");
    ESP_LOGI(TAG, " - CAN bus transmitting at 500 kbps");
    ESP_LOGI(TAG, " - HMI display running");
    ESP_LOGI(TAG, "===========================================");

    // Main loop - just monitor system health
    while (1) {
        // Print system info periodically
        ESP_LOGI(TAG, "System running... Free heap: %lu bytes", esp_get_free_heap_size());
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Log every 10 seconds
    }
}
