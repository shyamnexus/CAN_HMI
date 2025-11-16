#include <stdbool.h>

#include "board_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "board_i2c";
static bool s_i2c_initialized = false;

static esp_err_t board_i2c_configure(void)
{
    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(
        I2C_MASTER_NUM,
        conf.mode,
        I2C_MASTER_RX_BUF_DISABLE,
        I2C_MASTER_TX_BUF_DISABLE,
        0);

    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "I2C driver already installed, reusing existing instance");
        s_i2c_initialized = true;
        return ESP_OK;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return ret;
    }

    s_i2c_initialized = true;
    ESP_LOGI(TAG, "I2C driver installed");
    return ESP_OK;
}

esp_err_t board_i2c_init(void)
{
    if (s_i2c_initialized) {
        return ESP_OK;
    }
    return board_i2c_configure();
}

esp_err_t board_i2c_recover(void)
{
    if (s_i2c_initialized) {
        ESP_LOGW(TAG, "Re-initializing I2C driver after error");
        i2c_driver_delete(I2C_MASTER_NUM);
        s_i2c_initialized = false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return board_i2c_configure();
}

esp_err_t board_i2c_probe_device(uint8_t addr)
{
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "I2C bus not ready");

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "I2C device 0x%02X detected", addr);
    } else {
        ESP_LOGW(TAG, "I2C device 0x%02X not responding: %s", addr, esp_err_to_name(ret));
    }
    return ret;
}
