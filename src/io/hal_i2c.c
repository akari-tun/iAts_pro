#include "../src/target/target.h"

#pragma once

#include "hal_i2c.h"
#include <hal/err.h>

void hal_i2c_init(hal_i2c_config_t *cfg)
{

    HAL_ERR_ASSERT_OK(hal_i2c_bus_init(cfg->i2c_bus, cfg->sda, cfg->scl, I2C_MASTER_FREQ_HZ));

    cfg->is_init = 1;
}