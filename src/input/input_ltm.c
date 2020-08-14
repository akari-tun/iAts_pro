
#include <hal/log.h>
#include "input/input_ltm.h"

static const char *TAG = "Input.Ltm";

static bool input_ltm_update(void *input, void *data, time_micros_t now)
{
    input_ltm_t *input_ltm = input;

    bool updated = false;

    int ret = ltm_update(input_ltm->ltm, data);
    if (ret == 2)
    {
        input_ltm->last_frame_recv = now;
    }

    return updated;
}

static void input_ltm_close(void *input, void *config)
{
    input_ltm_t *input_ltm = input;
    serial_port_destroy(&input_ltm->serial_port);
    ltm_destroy(input_ltm->ltm);
    free(input_ltm->ltm);
}

static bool input_ltm_open(void *input, void *config)
{
    input_ltm_config_t *config_ltm = config;
    input_ltm_t *input_ltm = input;
    time_micros_t now = time_micros_now();

    input_ltm->inverted = false;
    input_ltm->last_frame_recv = now;
    input_ltm->enable_rx_deadline = TIME_MICROS_MAX;

    serial_port_config_t serial_config = {
        .baud_rate = config_ltm->baudrate,
        .tx_pin = config_ltm->tx,
        .rx_pin = config_ltm->rx,
        .tx_buffer_size = LTM_BUFFER_SIZE * 10,
        .rx_buffer_size = LTM_BUFFER_SIZE * 10,
        .parity = SERIAL_PARITY_DISABLE,
        .stop_bits = SERIAL_STOP_BITS_1,
        .inverted = input_ltm->inverted,
    };

    input_ltm->serial_port = serial_port_open(&serial_config);
    LOG_I(TAG, "Open with Baudrate: %d, TX: %s, RX: %s", config_ltm->baudrate, gpio_toa(config_ltm->tx), gpio_toa(config_ltm->rx));

    input_ltm->ltm = (ltm_t *)malloc(sizeof(ltm_t));

    ltm_init(input_ltm->ltm);

    input_ltm->ltm->io->write = (io_write_f)&serial_port_write;
    input_ltm->ltm->io->read = (io_read_f)&serial_port_read;
    input_ltm->ltm->io->flags = (io_flags_f)&serial_port_io_flags;
    input_ltm->ltm->io->data = input_ltm->serial_port;

    input_ltm->ltm->home_source = input_ltm->input.home_source;

    return true;
}

void input_ltm_init(input_ltm_t *input)
{
    input->serial_port = NULL;
    input->input.vtable = (input_vtable_t){
        .open = input_ltm_open,
        .update = input_ltm_update,
        .close = input_ltm_close,
    };
}