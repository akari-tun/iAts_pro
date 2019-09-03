#pragma once
#include "target/target.h"

#if defined(USE_RX5808)

#include <stdint.h>

#include <hal/adc.h>
#include <hal/gpio.h>

typedef struct rx5808_s
{
    adc_config_t rssi_adc_config;
    hal_spi_bus_t spi_bus;
    uint8_t active_channel;
    uint8_t rssi;
    uint16_t rssi_raw;
    uint8_t rssi_last;
} rx5808_t;

void rx5808_set_channel(rx5808_t *vrx, uint8_t channel);
void rx5808_update_rssi(rx5808_t *vrx);
void rx5808_init(rx5808_t *vrx);
void rx5808_update(rx5808_t *vrx);

#endif