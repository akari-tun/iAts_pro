#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "output/output.h"

#include "io/gpio.h"
#include "io/serial.h"

#include "protocols/pelco_d.h"

typedef struct output_pelco_d_config_s
{
    int baudrate;
    hal_gpio_t rx;
    hal_gpio_t tx;
} output_pelco_d_config_t;

typedef struct output_pelco_d_s
{
    output_t output;
    serial_port_t *serial_port;
    time_micros_t last_frame_recv;
    time_micros_t enable_rx_deadline;
    hal_gpio_t rx;
    hal_gpio_t tx;
    pelco_d_t *pelco_d;
    bool inverted;
    time_micros_t next_inversion_switch;
} output_pelco_d_t;

void output_pelco_d_init(output_pelco_d_t *output);