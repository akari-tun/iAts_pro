#pragma once

#include <hal/i2c.h>

#define I2C_MASTER_FREQ_HZ 400000

typedef struct hal_i2c_config_s
{
    hal_i2c_bus_t i2c_bus;
    hal_gpio_t sda;
    hal_gpio_t scl;
    bool is_init;
} hal_i2c_config_t;

void hal_i2c_init(hal_i2c_config_t *cfg);