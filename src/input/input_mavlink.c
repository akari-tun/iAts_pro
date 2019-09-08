
#include <hal/log.h>
#include "input/input_mavlink.h"

static const char *TAG = "Input.Mavlink";

// static mavlink_t mavlink;
// static io_t mavlink_io;

static bool input_mavlink_update(void *input, void *data, time_micros_t now)
{
    input_mavlink_t *input_mavlink = input;

    bool updated = false;

    int ret = mavlink_update(input_mavlink->mavlink, data);
    if (ret == 2)
    {
        input_mavlink->last_frame_recv = now;
    }

    return updated;
}

static void input_mavlink_close(void *input, void *config)
{
    input_mavlink_t *input_mavlink = input;
    serial_port_destroy(&input_mavlink->serial_port);
    mavlink_destroy(input_mavlink->mavlink);
    free(input_mavlink->mavlink);
}

static bool input_mavlink_open(void *input, void *config)
{
    input_mavlink_config_t *config_mavlink = config;
    input_mavlink_t *input_mavlink = input;
    time_micros_t now = time_micros_now();

    input_mavlink->inverted = false;
    input_mavlink->last_frame_recv = now;
    input_mavlink->enable_rx_deadline = TIME_MICROS_MAX;

    serial_port_config_t serial_config = {
        .baud_rate = config_mavlink->baudrate,
        .tx_pin = config_mavlink->tx,
        .rx_pin = config_mavlink->rx,
        .tx_buffer_size = MAVLINK_FRAME_SIZE_MAX,
        .rx_buffer_size = MAVLINK_FRAME_SIZE_MAX * 4,
        .parity = SERIAL_PARITY_DISABLE,
        .stop_bits = SERIAL_STOP_BITS_1,
        .inverted = input_mavlink->inverted,
    };

    input_mavlink->serial_port = serial_port_open(&serial_config);
    LOG_I(TAG, "Open with Baudrate: %d, TX: %s, RX: %s", config_mavlink->baudrate, gpio_toa(config_mavlink->tx), gpio_toa(config_mavlink->rx));

    input_mavlink->mavlink = (mavlink_t *)malloc(sizeof(mavlink_t));

    mavlink_init(input_mavlink->mavlink);

    input_mavlink->mavlink->io->write = (io_write_f)&serial_port_write;
    input_mavlink->mavlink->io->read = (io_read_f)&serial_port_read;
    input_mavlink->mavlink->io->flags = (io_flags_f)&serial_port_io_flags;
    input_mavlink->mavlink->io->data = input_mavlink->serial_port;

    return true;
}

void input_mavlink_init(input_mavlink_t *input)
{
    input->serial_port = NULL;
    input->input.vtable = (input_vtable_t){
        .open = input_mavlink_open,
        .update = input_mavlink_update,
        .close = input_mavlink_close,
    };
}