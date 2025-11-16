/*
 * CAN Transmission Implementation
 * Transmits thermocouple data over CAN/TWAI bus
 */

#include "can_transmit.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_i2c.h"
#include <string.h>

static const char *TAG = "CAN_TX";

// CAN/TWAI configuration
#ifndef CONFIG_EXAMPLE_TX_GPIO_NUM
#define CONFIG_EXAMPLE_TX_GPIO_NUM  20
#endif

#ifndef CONFIG_EXAMPLE_RX_GPIO_NUM
#define CONFIG_EXAMPLE_RX_GPIO_NUM  19
#endif

#ifndef CONFIG_EXAMPLE_CAN_BITRATE
#define CONFIG_EXAMPLE_CAN_BITRATE  500  // 500 kbps
#endif

static bool can_initialized = false;
static TaskHandle_t can_tx_task_handle = NULL;

/**
 * @brief Enable CAN transceiver on Waveshare board
 */
static esp_err_t enable_can_transceiver(void) {
    // When USB_SEL is HIGH, it enables FSUSB42UMX chip and connects CAN_TX/CAN_RX
    uint8_t write_buf = 0x01;
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, 
                                              I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to I2C device 0x24");
        return ret;
    }
    
    write_buf = 0x20;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, 
                                    I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to I2C device 0x38");
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t can_init(void) {
    if (can_initialized) {
        ESP_LOGW(TAG, "CAN already initialized");
        return ESP_OK;
    }

    // Initialize I2C for CAN transceiver control
    ESP_ERROR_CHECK(board_i2c_init());
    ESP_LOGI(TAG, "I2C initialized");
    
    // Enable CAN transceiver
    ESP_ERROR_CHECK(enable_can_transceiver());
    ESP_LOGI(TAG, "CAN transceiver enabled");

    // Configure TWAI timing for specified bitrate
    #if CONFIG_EXAMPLE_CAN_BITRATE == 1000
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    #elif CONFIG_EXAMPLE_CAN_BITRATE == 500
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    #elif CONFIG_EXAMPLE_CAN_BITRATE == 250
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    #elif CONFIG_EXAMPLE_CAN_BITRATE == 125
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    #else
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    #endif

    // Filter configuration - accept all messages
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // General configuration
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CONFIG_EXAMPLE_TX_GPIO_NUM, 
        CONFIG_EXAMPLE_RX_GPIO_NUM, 
        TWAI_MODE_NORMAL
    );

    // Install TWAI driver
    esp_err_t ret = twai_driver_install(&g_config, &t_config, &f_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver");
        return ret;
    }
    ESP_LOGI(TAG, "TWAI driver installed");

    // Start TWAI driver
    ret = twai_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start TWAI driver");
        return ret;
    }
    ESP_LOGI(TAG, "TWAI driver started");

    // Configure alerts
    uint32_t alerts = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | 
                     TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | 
                     TWAI_ALERT_BUS_ERROR;
    ret = twai_reconfigure_alerts(alerts, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reconfigure alerts");
        return ret;
    }

    can_initialized = true;
    ESP_LOGI(TAG, "CAN interface initialized successfully");
    
    return ESP_OK;
}

esp_err_t can_transmit_temperature(uint8_t channel, thermocouple_data_t *data) {
    if (!can_initialized) {
        ESP_LOGE(TAG, "CAN not initialized");
        return ESP_FAIL;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return ESP_FAIL;
    }

    // Prepare CAN message
    twai_message_t message;
    message.identifier = CAN_MSG_ID_TEMP_BASE + channel;
    message.data_length_code = 8;
    message.extd = 0;  // Standard ID
    message.rtr = 0;   // Data frame
    message.ss = 0;
    message.self = 0;
    message.dlc_non_comp = 0;

    // Pack temperature data into CAN message
    // Format: [Channel, Valid, Reserved, Reserved, Temperature (4 bytes float)]
    message.data[0] = channel;
    message.data[1] = data->is_valid ? 1 : 0;
    message.data[2] = 0;  // Reserved
    message.data[3] = 0;  // Reserved
    
    // Pack float temperature as 4 bytes
    memcpy(&message.data[4], &data->temperature, sizeof(float));

    // Transmit message
    esp_err_t ret = twai_transmit(&message, pdMS_TO_TICKS(100));
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "CH%d: %.2f°C transmitted", channel, data->temperature);
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Timeout transmitting channel %d", channel);
    } else {
        ESP_LOGE(TAG, "Failed to transmit channel %d", channel);
    }

    return ret;
}

esp_err_t can_transmit_all_temperatures(thermocouple_data_t *data, uint8_t num_channels) {
    if (!can_initialized) {
        ESP_LOGE(TAG, "CAN not initialized");
        return ESP_FAIL;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return ESP_FAIL;
    }

    // Transmit each channel
    for (uint8_t i = 0; i < num_channels; i++) {
        can_transmit_temperature(i, &data[i]);
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay between messages
    }

    return ESP_OK;
}

esp_err_t can_transmit_status(void) {
    if (!can_initialized) {
        return ESP_FAIL;
    }

    twai_message_t message;
    message.identifier = CAN_MSG_ID_STATUS;
    message.data_length_code = 8;
    message.extd = 0;
    message.rtr = 0;
    message.ss = 0;
    message.self = 0;
    message.dlc_non_comp = 0;

    // Get TWAI status
    twai_status_info_t status;
    twai_get_status_info(&status);

    // Pack status data
    message.data[0] = (status.state == TWAI_STATE_RUNNING) ? 1 : 0;
    message.data[1] = status.msgs_to_tx;
    message.data[2] = status.msgs_to_rx;
    message.data[3] = status.tx_error_counter;
    message.data[4] = status.rx_error_counter;
    message.data[5] = status.tx_failed_count;
    message.data[6] = status.rx_missed_count;
    message.data[7] = status.bus_error_count;

    return twai_transmit(&message, pdMS_TO_TICKS(100));
}

/**
 * @brief CAN transmission task
 */
static void can_tx_task(void *arg) {
    thermocouple_data_t temp_data[MAX_THERMOCOUPLE_CHANNELS];
    
    ESP_LOGI(TAG, "CAN transmission task started");

    while (1) {
        // Read all thermocouple channels
        if (thermocouple_read_all(temp_data, MAX_THERMOCOUPLE_CHANNELS) == ESP_OK) {
            // Transmit all temperatures
            can_transmit_all_temperatures(temp_data, MAX_THERMOCOUPLE_CHANNELS);
            
            // Transmit status
            can_transmit_status();
        }

        // Check for alerts
        uint32_t alerts;
        twai_read_alerts(&alerts, pdMS_TO_TICKS(10));
        
        if (alerts & TWAI_ALERT_TX_FAILED) {
            ESP_LOGW(TAG, "CAN TX failed alert");
        }
        if (alerts & TWAI_ALERT_BUS_ERROR) {
            ESP_LOGW(TAG, "CAN bus error alert");
        }

        vTaskDelay(pdMS_TO_TICKS(CAN_TX_RATE_MS));
    }
}

esp_err_t can_start_transmission_task(void) {
    if (can_tx_task_handle != NULL) {
        ESP_LOGW(TAG, "CAN transmission task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(can_tx_task, "can_tx_task", 4096, NULL, 5, &can_tx_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CAN transmission task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "CAN transmission task started");
    return ESP_OK;
}

esp_err_t can_stop_transmission_task(void) {
    if (can_tx_task_handle == NULL) {
        ESP_LOGW(TAG, "CAN transmission task not running");
        return ESP_OK;
    }

    vTaskDelete(can_tx_task_handle);
    can_tx_task_handle = NULL;
    
    ESP_LOGI(TAG, "CAN transmission task stopped");
    return ESP_OK;
}
