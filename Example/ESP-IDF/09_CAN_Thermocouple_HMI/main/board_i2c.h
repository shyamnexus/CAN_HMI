#pragma once

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MASTER_SCL_IO           9
#define I2C_MASTER_SDA_IO           8
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

/**
 * @brief Initialize the shared Waveshare board I2C bus.
 *
 * Safe to call multiple times; the underlying driver is installed only once.
 *
 * @return ESP_OK on success or already initialized, error otherwise.
 */
esp_err_t board_i2c_init(void);

#ifdef __cplusplus
}
#endif
