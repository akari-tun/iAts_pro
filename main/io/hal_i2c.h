#pragma once

#include <hal/i2c.h>

#define I2C_MASTER_FREQ_HZ 400000

#define ACK_CHECK_EN                       true             /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      false            /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

typedef struct hal_i2c_config_s
{
    hal_i2c_bus_t i2c_bus;
    hal_gpio_t sda;
    hal_gpio_t scl;
    bool is_init;
    uint32_t freq_hz;
    SemaphoreHandle_t xSemaphore;
} hal_i2c_config_t;

void hal_i2c_init(hal_i2c_config_t *cfg);
void hal_i2c_deinit(hal_i2c_config_t *cfg);
void hal_i2c_cmd_take(hal_i2c_bus_t i2c_bus);
void hal_i2c_cmd_give(hal_i2c_bus_t i2c_bus);