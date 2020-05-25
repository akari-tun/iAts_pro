#include <stdio.h>

#include <hal/err.h>
#include <hal/log.h>
#include "mpu9250_i2c.h"

static const char *TAG = "mpu9250.i2c";

static bool mpu9250_handl_error(mpu9250_i2c_config_t *cfg, hal_err_t err)
{
    if (err != HAL_ERR_NONE)
    {
        if (cfg->rst != HAL_GPIO_NONE)
        {
            hal_gpio_set_level(cfg->rst, HAL_GPIO_LOW);
        }
        HAL_ERR_ASSERT_OK(hal_i2c_bus_deinit(cfg->i2c_cfg->i2c_bus));

        LOG_E(TAG, "%d", err);
        
        return false;
    }

    hal_i2c_cmd_give(cfg->i2c_cfg->i2c_bus);
    return true;
}

void mpu9250_i2c_init(mpu9250_i2c_config_t *cfg)
{
    if (!cfg->i2c_cfg->is_init)
    {
        LOG_I(TAG, "init i2c");
        HAL_ERR_ASSERT_OK(hal_i2c_bus_init(cfg->i2c_cfg->i2c_bus, cfg->i2c_cfg->sda, cfg->i2c_cfg->scl, cfg->i2c_cfg->freq_hz));
    }
}

bool mpu9250_i2c_read(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size)
{
    hal_i2c_cmd_take(cfg->i2c_cfg->i2c_bus);
    hal_i2c_cmd_t cmd;
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_init(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_start(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_write_byte(&cmd, HAL_I2C_READ_ADDR(cfg->addr), ACK_CHECK_EN));
    if (size > 1)
    {
        HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_read(&cmd, data, size, ACK_VAL));
    }
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_read_byte(&cmd, data, NACK_VAL));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_stop(&cmd));

    hal_err_t err = hal_i2c_cmd_master_exec(cfg->i2c_cfg->i2c_bus, &cmd);
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_destroy(&cmd));

    return mpu9250_handl_error(cfg, err);
}

bool mpu9250_i2c_write(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size)
{
    hal_i2c_cmd_take(cfg->i2c_cfg->i2c_bus);
    hal_i2c_cmd_t cmd;
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_init(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_start(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_write_byte(&cmd, HAL_I2C_WRITE_ADDR(cfg->addr), ACK_CHECK_EN));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_write(&cmd, data, size, true));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_stop(&cmd));

    hal_err_t err = hal_i2c_cmd_master_exec(cfg->i2c_cfg->i2c_bus, &cmd);
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_destroy(&cmd));

    return mpu9250_handl_error(cfg, err);;
}

bool ak8963_i2c_read(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size)
{
    hal_i2c_cmd_take(cfg->i2c_cfg->i2c_bus);
    hal_i2c_cmd_t cmd;
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_init(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_start(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_write_byte(&cmd, HAL_I2C_READ_ADDR(cfg->ak8963_addr), ACK_CHECK_EN));
    if (size > 1)
    {
        HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_read(&cmd, data, size, ACK_VAL));
    }
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_read_byte(&cmd, data, NACK_VAL));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_stop(&cmd));

    hal_err_t err = hal_i2c_cmd_master_exec(cfg->i2c_cfg->i2c_bus, &cmd);
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_destroy(&cmd));

    return mpu9250_handl_error(cfg, err);;
}

bool ak8963_i2c_write(mpu9250_i2c_config_t *cfg, uint8_t* data, uint32_t size)
{
    hal_i2c_cmd_take(cfg->i2c_cfg->i2c_bus);
    hal_i2c_cmd_t cmd;
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_init(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_start(&cmd));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_write_byte(&cmd, HAL_I2C_WRITE_ADDR(cfg->ak8963_addr), ACK_CHECK_EN));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_write(&cmd, data, size, ACK_CHECK_EN));
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_master_stop(&cmd));

    hal_err_t err = hal_i2c_cmd_master_exec(cfg->i2c_cfg->i2c_bus, &cmd);
    HAL_ERR_ASSERT_OK(hal_i2c_cmd_destroy(&cmd));

    return mpu9250_handl_error(cfg, err);;
}