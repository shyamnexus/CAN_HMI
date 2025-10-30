/*
 * Thermocouple Interface Implementation
 * This module handles reading K-type thermocouple data
 * Currently uses simulated data, but can be extended to support real sensors
 */

#include "thermocouple.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "THERMOCOUPLE";

// Storage for thermocouple readings
static thermocouple_data_t channel_data[MAX_THERMOCOUPLE_CHANNELS];
static bool initialized = false;

// Simulated base temperatures for each channel (for demo purposes)
static float base_temps[MAX_THERMOCOUPLE_CHANNELS] = {
    25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0,
    65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0
};

/**
 * @brief Simulate thermocouple reading with realistic variations
 */
static float simulate_temperature(uint8_t channel) {
    // Get current time for random-like variations
    uint64_t time_us = esp_timer_get_time();
    float time_sec = time_us / 1000000.0f;
    
    // Add sinusoidal variation for realistic temperature fluctuation
    float variation = sinf(time_sec * 0.5f + channel * 0.3f) * 2.0f;
    
    // Add small random-like noise
    float noise = ((time_us % 100) - 50) / 50.0f;
    
    return base_temps[channel] + variation + noise;
}

esp_err_t thermocouple_init(void) {
    if (initialized) {
        ESP_LOGW(TAG, "Thermocouple already initialized");
        return ESP_OK;
    }

    // Initialize all channels
    for (int i = 0; i < MAX_THERMOCOUPLE_CHANNELS; i++) {
        channel_data[i].temperature = 0.0f;
        channel_data[i].is_valid = false;
        channel_data[i].error_count = 0;
    }

    // TODO: Initialize SPI or I2C for real thermocouple amplifiers
    // For now, we'll use simulated data
    
    ESP_LOGI(TAG, "Thermocouple interface initialized (simulated mode)");
    initialized = true;
    
    return ESP_OK;
}

esp_err_t thermocouple_read_channel(uint8_t channel, thermocouple_data_t *data) {
    if (!initialized) {
        ESP_LOGE(TAG, "Thermocouple not initialized");
        return ESP_FAIL;
    }

    if (channel >= MAX_THERMOCOUPLE_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return ESP_FAIL;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return ESP_FAIL;
    }

    // In simulation mode, generate realistic temperature data
    #ifdef CONFIG_EXAMPLE_USE_SIMULATED_DATA
    data->temperature = simulate_temperature(channel);
    data->is_valid = true;
    data->error_count = 0;
    #else
    // TODO: Read from actual thermocouple hardware
    // Example for MAX31855 or MAX31856 via SPI
    // Or MCP9600 via I2C
    data->temperature = 0.0f;
    data->is_valid = false;
    data->error_count++;
    #endif

    // Store the reading
    channel_data[channel] = *data;

    return ESP_OK;
}

esp_err_t thermocouple_read_all(thermocouple_data_t *data, uint8_t num_channels) {
    if (!initialized) {
        ESP_LOGE(TAG, "Thermocouple not initialized");
        return ESP_FAIL;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return ESP_FAIL;
    }

    if (num_channels > MAX_THERMOCOUPLE_CHANNELS) {
        num_channels = MAX_THERMOCOUPLE_CHANNELS;
    }

    // Read all channels
    for (uint8_t i = 0; i < num_channels; i++) {
        thermocouple_read_channel(i, &data[i]);
    }

    return ESP_OK;
}

float thermocouple_get_temperature(uint8_t channel) {
    if (!initialized || channel >= MAX_THERMOCOUPLE_CHANNELS) {
        return 0.0f;
    }
    return channel_data[channel].temperature;
}

bool thermocouple_is_valid(uint8_t channel) {
    if (!initialized || channel >= MAX_THERMOCOUPLE_CHANNELS) {
        return false;
    }
    return channel_data[channel].is_valid;
}
