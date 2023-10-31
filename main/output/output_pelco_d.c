
#include <hal/log.h>
#include "output/output_pelco_d.h"

static const char *TAG = "output.pelco_d";

static bool output_pelco_d_update(void *output, void *data, time_micros_t now)
{
    output_pelco_d_t *output_pelco_d = output;

    bool updated = false;
    int ret = pelco_d_update(output_pelco_d->pelco_d, data);
    
    if (ret == 2)
    {
        updated = true;
        output_pelco_d->last_frame_recv = now;
    }

    return updated;
}

static void output_pelco_d_close(void *output, void *config)
{
    output_pelco_d_t *output_pelco_d = output;
    serial_port_destroy(&output_pelco_d->serial_port);
    pelco_d_destroy(output_pelco_d->pelco_d);
    free(output_pelco_d->pelco_d);
}

static bool output_pelco_d_open(void *output, void *config)
{
    output_pelco_d_config_t *config_pelco_d = config;
    output_pelco_d_t *output_pelco_d = output;
    time_micros_t now = time_micros_now();

    output_pelco_d->inverted = false;
    output_pelco_d->last_frame_recv = now;
    output_pelco_d->enable_rx_deadline = TIME_MICROS_MAX;

    serial_port_config_t serial_config = {
        .baud_rate = config_pelco_d->baudrate,
        .tx_pin = config_pelco_d->tx,
        .rx_pin = config_pelco_d->rx,
        .tx_buffer_size = PELCO_D_FRAME_SIZE * 20,
        .rx_buffer_size = PELCO_D_FRAME_SIZE * 20,
        .parity = SERIAL_PARITY_DISABLE,
        .stop_bits = SERIAL_STOP_BITS_1,
        .inverted = output_pelco_d->inverted,
    };

    output_pelco_d->serial_port = serial_port_open(&serial_config);
    LOG_I(TAG, "Open with Baudrate: %d, TX: %s, RX: %s", config_pelco_d->baudrate, gpio_toa(config_pelco_d->tx), gpio_toa(config_pelco_d->rx));

    output_pelco_d->pelco_d = (pelco_d_t *)malloc(sizeof(pelco_d_t));

    pelco_d_init(output_pelco_d->pelco_d);

    output_pelco_d->pelco_d->io->write = (io_write_f)&serial_port_write;
    output_pelco_d->pelco_d->io->read = (io_read_f)&serial_port_read;
    output_pelco_d->pelco_d->io->flags = (io_flags_f)&serial_port_io_flags;
    output_pelco_d->pelco_d->io->data = output_pelco_d->serial_port;

    return true;
}

void output_pelco_d_init(output_pelco_d_t *output)
{
    output->serial_port = NULL;
    output->output.vtable = (output_vtable_t){
        .open = output_pelco_d_open,
        .update = output_pelco_d_update,
        .close = output_pelco_d_close,
    };
}