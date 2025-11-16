#include <stdbool.h>

#include "board_i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "board_i2c";
static bool s_i2c_initialized = false;

esp_err_t board_i2c_init(void)
{
    if (s_i2c_initialized) {
        return ESP_OK;
    }

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
