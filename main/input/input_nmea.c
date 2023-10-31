#include <hal/log.h>
#include "input/input_nmea.h"

static const char *TAG = "Input.NMEA";

static bool input_nmea_update(void *input, void *data, time_micros_t now)
{
    input_nmea_t *input_nmea = input;

    bool updated = false;

    int ret = nmea_update(input_nmea->nmea, data);
    if (ret == 2)
    {
        input_nmea->last_frame_recv = now;
    }

    return updated;
}

static void input_nmea_close(void *input, void *config)
{
    input_nmea_t *input_nmea = input;
    serial_port_destroy(&input_nmea->serial_port);
    nmea_destroy(input_nmea->nmea);
    free(input_nmea->nmea);
}

static bool input_nmea_open(void *input, void *config)
{
    input_nmea_config_t *config_nmea = config;
    input_nmea_t *input_nmea = input;
    time_micros_t now = time_micros_now();

    input_nmea->inverted = false;
    input_nmea->last_frame_recv = now;
    input_nmea->enable_rx_deadline = TIME_MICROS_MAX;

    serial_port_config_t serial_config = {
        .baud_rate = config_nmea->baudrate,
        .tx_pin = config_nmea->tx,
        .rx_pin = config_nmea->rx,
        .tx_buffer_size = NMEA_FRAME_SIZE_MAX,
        .rx_buffer_size = NMEA_FRAME_SIZE_MAX * 2,
        .parity = SERIAL_PARITY_DISABLE,
        .stop_bits = SERIAL_STOP_BITS_1,
        .inverted = input_nmea->inverted,
    };

    input_nmea->serial_port = serial_port_open(&serial_config);
    LOG_I(TAG, "Open with Baudrate: %d, TX: %s, RX: %s", config_nmea->baudrate, gpio_toa(config_nmea->tx), gpio_toa(config_nmea->rx));

    input_nmea->nmea = (nmea_t *)malloc(sizeof(nmea_t));

    nmea_init(input_nmea->nmea);

    input_nmea->nmea->io->write = (io_write_f)&serial_port_write;
    input_nmea->nmea->io->read = (io_read_f)&serial_port_read;
    input_nmea->nmea->io->flags = (io_flags_f)&serial_port_io_flags;
    input_nmea->nmea->io->data = input_nmea->serial_port;

    input_nmea->nmea->home_source = input_nmea->input.home_source;

    return true;
}

void input_nmea_init(input_nmea_t *input)
{
    input->serial_port = NULL;
    input->input.vtable = (input_vtable_t){
        .open = input_nmea_open,
        .update = input_nmea_update,
        .close = input_nmea_close,
    };
}