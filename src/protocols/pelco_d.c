#include "pelco_d.h"

#include <hal/log.h>
#include "atp.h"
#include "tracker/tracker.h"

static const char *TAG = "Protocol.PELCO_D";

static uint8_t checksum(pelco_d_t *pelco_d)
{
    uint32_t ret = 0;

    for (uint8_t i = 1; i <= 5; i++)
    {
        ret += pelco_d->send_buf[i];
    }
    
    return (ret % 256);
}

static void set_pan(pelco_d_t *pelco_d, uint16_t degree)
{
    degree *= 100;
    
    pelco_d->send_buf[0] = PELCO_D_SYNC_BYTE;
    pelco_d->send_buf[1] = PELCO_D_ADDRESS;
    pelco_d->send_buf[2] = (SET_PAN_POSITION >> 8 * 1) & 0xFF;
    pelco_d->send_buf[3] = (SET_PAN_POSITION >> 8 * 0) & 0xFF;
    pelco_d->send_buf[4] = (degree >> 8 * 1) & 0xFF;
    pelco_d->send_buf[5] = (degree >> 8 * 0) & 0xFF;
    pelco_d->send_buf[6] = checksum(pelco_d);
}

static void set_tilt(pelco_d_t *pelco_d, uint16_t degree)
{
    degree *= 100;

    pelco_d->send_buf[0] = PELCO_D_SYNC_BYTE;
    pelco_d->send_buf[1] = PELCO_D_ADDRESS;
    pelco_d->send_buf[2] = (SET_TILT_POSITION >> 8 * 1) & 0xFF;
    pelco_d->send_buf[3] = (SET_TILT_POSITION >> 8 * 0) & 0xFF;
    pelco_d->send_buf[4] = (degree >> 8 * 1) & 0xFF;
    pelco_d->send_buf[5] = (degree >> 8 * 0) & 0xFF;
    pelco_d->send_buf[6] = checksum(pelco_d);
}

void pelco_d_init(pelco_d_t *pelco_d)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    pelco_d->io = (io_t *)malloc(sizeof(io_t));;
    pelco_d->buf_pos = 0;

    memset(&pelco_d->recive_buf, 0, sizeof(pelco_d->recive_buf));
    memset(&pelco_d->send_buf, 0, sizeof(pelco_d->send_buf));

    LOG_I(TAG, "Initialized");
}

int pelco_d_update(pelco_d_t *pelco_d, void *data)
{
    int rem = sizeof(pelco_d->recive_buf)  - pelco_d->buf_pos;
    int n = io_read(pelco_d->io, &pelco_d->recive_buf[pelco_d->buf_pos], rem, 0);

    uint8_t ret = 0;

    tracker_t *t = (tracker_t *)data;
    
    if (t->servo->internal.pan.currtent_degree != pelco_d->pan_degree)
    {
        set_pan(pelco_d, t->servo->internal.pan.currtent_degree);
        n = io_write(pelco_d->io, &pelco_d->send_buf, PELCO_D_FRAME_SIZE);

        if (n > 0)
        {   
            pelco_d->pan_degree = t->servo->internal.pan.currtent_degree;
            ret = 1;

            LOG_I(TAG, "Curr_Deg:%d | Pan_Deg:%d | Write: %d", t->servo->internal.pan.currtent_degree, pelco_d->pan_degree, n);
        }
    }

    if (t->servo->internal.tilt.currtent_degree != pelco_d->tilt_degree)
    {
        set_tilt(pelco_d, 360 - t->servo->internal.tilt.currtent_degree);
        n = io_write(pelco_d->io, &pelco_d->send_buf, PELCO_D_FRAME_SIZE);

        if (n > 0)
        {   
            pelco_d->tilt_degree = t->servo->internal.tilt.currtent_degree;
            ret = 1;

            LOG_I(TAG, "Curr_Deg:%d | Tilt_Deg:%d | Write: %d", t->servo->internal.tilt.currtent_degree, pelco_d->tilt_degree, n);
        }
    }

    return ret;
}

void pelco_d_destroy(pelco_d_t *pelco_d)
{
    free(pelco_d->io);
}