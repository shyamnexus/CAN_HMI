/*
 * CAN Transmission Module
 * Handles transmission of thermocouple data over CAN bus
 */

#ifndef CAN_TRANSMIT_H
#define CAN_TRANSMIT_H

#include <stdint.h>
#include "esp_err.h"
#include "thermocouple.h"

#ifdef __cplusplus
extern "C" {
#endif

// CAN message IDs for thermocouple data
#define CAN_MSG_ID_TEMP_BASE        0x100  // Base ID for temperature messages
#define CAN_MSG_ID_STATUS           0x110  // Status message ID
#define CAN_MSG_ID_HEARTBEAT        0x111  // Heartbeat message ID

// CAN transmission rate
#define CAN_TX_RATE_MS              500    // Transmit every 500ms

/**
 * @brief Initialize CAN interface
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t can_init(void);

/**
 * @brief Transmit thermocouple data over CAN
 * 
 * @param channel Channel number (0-15)
 * @param data Temperature data to transmit
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t can_transmit_temperature(uint8_t channel, thermocouple_data_t *data);

/**
 * @brief Transmit all thermocouple channels over CAN
 * 
 * @param data Array of temperature data for all channels
 * @param num_channels Number of channels to transmit
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t can_transmit_all_temperatures(thermocouple_data_t *data, uint8_t num_channels);

/**
 * @brief Transmit system status over CAN
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t can_transmit_status(void);

/**
 * @brief Start CAN transmission task
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t can_start_transmission_task(void);

/**
 * @brief Stop CAN transmission task
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t can_stop_transmission_task(void);

#ifdef __cplusplus
}
#endif

#endif // CAN_TRANSMIT_H
