#include <stdbool.h>

#include "board_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "board_i2c";
static bool s_i2c_initialized = false;
static i2c_master_bus_handle_t s_i2c_bus = NULL;

static esp_err_t board_i2c_configure(void)
{
    if (s_i2c_initialized && s_i2c_bus) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &s_i2c_bus);
    if (ret == ESP_OK) {
        s_i2c_initialized = true;
        ESP_LOGI(TAG, "I2C master bus initialized");
    } else {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t board_i2c_init(void)
{
    return board_i2c_configure();
}

esp_err_t board_i2c_recover(void)
{
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "I2C bus not ready");

    /**
     * The new I2C master driver already performs bus arbitration and clock
     * stretching handling internally. In most cases the peripheral recovers
     * once the master issues a stop condition and retries after a short delay.
     * Add a small guard delay here so higher level callers can yield before
     * retrying the transaction.
     */
    vTaskDelay(pdMS_TO_TICKS(5));
    return ESP_OK;
}

esp_err_t board_i2c_probe_device(uint8_t addr)
{
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "I2C bus not ready");

    esp_err_t ret = i2c_master_probe(s_i2c_bus, addr, I2C_MASTER_TIMEOUT_MS);
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "I2C device 0x%02X detected", addr);
    } else {
        ESP_LOGW(TAG, "I2C device 0x%02X not responding: %s", addr, esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t board_i2c_get_bus(i2c_master_bus_handle_t *out_bus)
{
    ESP_RETURN_ON_FALSE(out_bus, ESP_ERR_INVALID_ARG, TAG, "Output pointer is NULL");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "I2C bus not ready");
    *out_bus = s_i2c_bus;
    return ESP_OK;
}

static esp_err_t board_i2c_open_device(uint8_t addr, i2c_master_dev_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "Device handle pointer is NULL");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "I2C bus not ready");

    const i2c_device_config_t dev_cfg = {
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    return i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, out_handle);
}

esp_err_t board_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(data && len > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid data buffer");

    i2c_master_dev_handle_t dev = NULL;
    ESP_RETURN_ON_ERROR(board_i2c_open_device(addr, &dev), TAG, "Failed to add device 0x%02X", addr);

    esp_err_t ret = i2c_master_transmit(dev, data, len, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C write to 0x%02X failed: %s", addr, esp_err_to_name(ret));
    }

    esp_err_t rm_ret = i2c_master_bus_rm_device(dev);
    if (rm_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to remove I2C device 0x%02X: %s", addr, esp_err_to_name(rm_ret));
        if (ret == ESP_OK) {
            ret = rm_ret;
        }
    }

    return ret;
}

esp_err_t board_i2c_write_byte(uint8_t addr, uint8_t byte)
{
    return board_i2c_write(addr, &byte, 1);
}
