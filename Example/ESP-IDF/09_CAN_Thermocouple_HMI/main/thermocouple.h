/*
 * Thermocouple Interface Module
 * Supports simulated and real K-type thermocouple sensors
 */

#ifndef THERMOCOUPLE_H
#define THERMOCOUPLE_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THERMOCOUPLE_CHANNELS 16

// Thermocouple data structure
typedef struct {
    float temperature;      // Temperature in Celsius
    bool is_valid;         // Data validity flag
    uint32_t error_count;  // Error counter
} thermocouple_data_t;

/**
 * @brief Initialize thermocouple interface
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t thermocouple_init(void);

/**
 * @brief Read temperature from a specific channel
 * 
 * @param channel Channel number (0-15)
 * @param data Pointer to store temperature data
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t thermocouple_read_channel(uint8_t channel, thermocouple_data_t *data);

/**
 * @brief Read all thermocouple channels
 * 
 * @param data Array to store temperature data for all channels
 * @param num_channels Number of channels to read
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t thermocouple_read_all(thermocouple_data_t *data, uint8_t num_channels);

/**
 * @brief Get the last read temperature for a channel
 * 
 * @param channel Channel number (0-15)
 * @return Temperature in Celsius
 */
float thermocouple_get_temperature(uint8_t channel);

/**
 * @brief Check if channel data is valid
 * 
 * @param channel Channel number (0-15)
 * @return true if valid, false otherwise
 */
bool thermocouple_is_valid(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif // THERMOCOUPLE_H
