#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "input/input.h"

#include "io/gpio.h"
#include "io/serial.h"

#include "protocols/mavlink.h"

typedef struct input_mavlink_config_s
{
    int baudrate;
    hal_gpio_t rx;
    hal_gpio_t tx;
} input_mavlink_config_t;

typedef struct input_mavlink_s
{
    input_t input;
    serial_port_t *serial_port;
    time_micros_t last_frame_recv;
    time_micros_t enable_rx_deadline;
    hal_gpio_t rx;
    hal_gpio_t tx;
    mavlink_t *mavlink;
    bool inverted;
    time_micros_t next_inversion_switch;
} input_mavlink_t;

void input_mavlink_init(input_mavlink_t *input);