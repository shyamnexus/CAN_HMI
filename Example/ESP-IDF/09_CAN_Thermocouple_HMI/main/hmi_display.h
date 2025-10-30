/*
 * HMI Display Module
 * Manages the LVGL GUI for displaying thermocouple data
 */

#ifndef HMI_DISPLAY_H
#define HMI_DISPLAY_H

#include "lvgl.h"
#include "thermocouple.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize HMI display
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t hmi_display_init(void);

/**
 * @brief Update temperature display for a specific channel
 * 
 * @param channel Channel number (0-15)
 * @param data Temperature data to display
 */
void hmi_display_update_channel(uint8_t channel, thermocouple_data_t *data);

/**
 * @brief Update all channel displays
 * 
 * @param data Array of temperature data for all channels
 * @param num_channels Number of channels to update
 */
void hmi_display_update_all(thermocouple_data_t *data, uint8_t num_channels);

/**
 * @brief Update CAN status indicator
 * 
 * @param connected true if CAN is connected and transmitting
 */
void hmi_display_update_can_status(bool connected);

/**
 * @brief Start HMI update task
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t hmi_display_start_update_task(void);

#ifdef __cplusplus
}
#endif

#endif // HMI_DISPLAY_H
