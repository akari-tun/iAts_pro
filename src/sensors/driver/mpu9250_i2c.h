#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <hal/gpio.h>
//#include <hal/i2c.h>
#include "io/hal_i2c.h"

typedef struct mpu9250_i2c_config_s
{
    hal_i2c_config_t *i2c_cfg;
    hal_gpio_t rst;
    uint8_t addr;
    uint8_t ak8963_addr;
} mpu9250_i2c_config_t;

void mpu9250_i2c_init(mpu9250_i2c_config_t *cfg);
bool mpu9250_i2c_read(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size);
bool mpu9250_i2c_write(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size);
bool ak8963_i2c_read(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size);
bool ak8963_i2c_write(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size);