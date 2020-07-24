#include "../src/target/target.h"

#pragma once

#include "hal_i2c.h"
#include <hal/err.h>
#include "freertos/semphr.h"

static SemaphoreHandle_t _mutexs[2];

void hal_i2c_init(hal_i2c_config_t *cfg)
{
    HAL_ERR_ASSERT_OK(hal_i2c_bus_init(cfg->i2c_bus, cfg->sda, cfg->scl, cfg->freq_hz));

    _mutexs[cfg->i2c_bus] = xSemaphoreCreateMutex();
    //_mutexs[1] = xSemaphoreCreateMutex();
    cfg->xSemaphore = xSemaphoreCreateMutex();

    cfg->is_init = 1;
}

void hal_i2c_deinit(hal_i2c_config_t *cfg)
{
    HAL_ERR_ASSERT_OK(hal_i2c_bus_deinit(cfg->i2c_bus));
    cfg->is_init = 0;
}

void hal_i2c_cmd_take(hal_i2c_bus_t i2c_bus)
{
    xSemaphoreTake(_mutexs[i2c_bus], portMAX_DELAY);
}

void hal_i2c_cmd_give(hal_i2c_bus_t i2c_bus)
{
    xSemaphoreGive(_mutexs[i2c_bus]);
}