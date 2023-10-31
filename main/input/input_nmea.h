#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "input/input.h"

#include "io/gpio.h"
#include "io/serial.h"

#include "protocols/nmea.h"

typedef struct input_nmea_config_s
{
    int baudrate;
    hal_gpio_t rx;
    hal_gpio_t tx;
} input_nmea_config_t;

typedef struct input_nmea_s
{
    input_t input;
    serial_port_t *serial_port;
    time_micros_t last_frame_recv;
    time_micros_t enable_rx_deadline;
    hal_gpio_t rx;
    hal_gpio_t tx;
    nmea_t *nmea;
    bool inverted;
    time_micros_t next_inversion_switch;
} input_nmea_t;

void input_nmea_init(input_nmea_t *input);