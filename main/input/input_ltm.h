#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "input/input.h"

#include "io/gpio.h"
#include "io/serial.h"

#include "protocols/ltm.h"

typedef struct input_ltm_config_s
{
    int baudrate;
    hal_gpio_t rx;
    hal_gpio_t tx;
} input_ltm_config_t;

typedef struct input_ltm_s
{
    input_t input;
    serial_port_t *serial_port;
    time_micros_t last_frame_recv;
    time_micros_t enable_rx_deadline;
    hal_gpio_t rx;
    hal_gpio_t tx;
    ltm_t *ltm;
    bool inverted;
    time_micros_t next_inversion_switch;
} input_ltm_t;

void input_ltm_init(input_ltm_t *input);