#include "ltm.h"

#include <hal/log.h>
#include "atp.h"

static const char *TAG = "Protocol.Ltm";
static ltm_gframe_t gframe;
static ltm_aframe_t aframe;
static ltm_sframe_t sframe;
static ltm_oframe_t oframe;
static ltm_nframe_t nframe;
static ltm_xframe_t xframe;

void ltm_init(ltm_t *ltm)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ltm->io = (io_t *)malloc(sizeof(io_t));
    ltm->buf_pos = 0;
    ltm->status = LTM_IDLE;
    ltm->gframe = &gframe;
    ltm->aframe = &aframe;
    ltm->sframe = &sframe;
    ltm->oframe = &oframe;
    ltm->nframe = &nframe;
    ltm->xframe = &xframe;

    memset(&ltm->buf, 0, sizeof(ltm->buf));

    LOG_I(TAG, "Initialized");
}

int ltm_update(ltm_t *ltm, void *data)
{
    int rem = sizeof(ltm->buf)  - ltm->buf_pos;
    int n = io_read(ltm->io, &ltm->buf[ltm->buf_pos], rem, 0);

    uint8_t ret = 0;

    if (n <= 0 && ltm->buf_pos == 0)
    {
        return ret;
    }

    ltm->buf_pos += n;
    LOG_D(TAG, "Read %d bytes, buf at %u", n, ltm->buf_pos);

    int start = 0;
    int end = ltm->buf_pos;
    ret = 1;

    while (start < ltm->buf_pos)
    {
        if (ltm_decode(ltm, ltm->buf[start++]))
        {
            time_micros_t now = time_micros_now();
            atp_t *atp = (atp_t *)data;

            switch (ltm->function)
            {
                case LTM_GFRAME:
                    ATP_SET_I32(TAG_PLANE_LONGITUDE,  ltm->gframe->longitude, now);
                    ATP_SET_I32(TAG_PLANE_LATITUDE, ltm->gframe->latitude, now);
                    ATP_SET_I32(TAG_PLANE_ALTITUDE, ltm->gframe->altitude, now);
                    ATP_SET_I16(TAG_PLANE_STAR, (int16_t)(ltm->gframe->sats >> 2) & 0xFF, now);
                    ATP_SET_U8(TAG_PLANE_FIX, (uint8_t)(ltm->gframe->sats & 0b00000011), now);
                    atp->tag_value_changed(atp->tracker, TAG_PLANE_LATITUDE);
                    atp->tag_value_changed(atp->tracker, TAG_PLANE_LONGITUDE);

                    // LOG_I(TAG, "ALT: %d", ltm->gframe->altitude);
                    // LOG_I(TAG, "GS: %d", ltm->gframe->ground_speed);
                    // LOG_I(TAG, "Sats: %d", (ltm->gframe->sats >> 2) & 0xFF);
                    // LOG_I(TAG, "Fix: %d", (uint8_t)(ltm->gframe->sats & 0b00000011));
                    // LOG_I(TAG, "SIZE OF: %d", sizeof(ltm_gframe_t));
                    break;
            }

            break;
        }
    }

    if (start > 0)
    {
        LOG_D(TAG, "Consumed %d bytes", start);
        LOG_BUFFER_D(TAG, &ltm->buf[0], start);
        if (start != ltm->buf_pos)
        {
            // Got some data at the end that we need to copy
            memmove(ltm->buf, &ltm->buf[start], end - start);
        }
        ltm->buf_pos -= start;
    }

    return ret;
}

bool ltm_decode(ltm_t *ltm, uint8_t c)
{
    bool ret = false;

    if (ltm->status == LTM_IDLE && c == LTM_START1)
    {
        ltm->status = LTM_STATE_START1;
        LOG_D(TAG, "Leader_1: %d", c);
    }
    else if(ltm->status == LTM_STATE_START1 && c == LTM_START2)
    {
        ltm->status = LTM_STATE_START2;
        LOG_D(TAG, "Leader_2: %d", c);
    }
    else if (ltm->status == LTM_STATE_START2)
    {
        ltm->status = LTM_STATE_MSGTYPE;
        ltm->function = c;
        ltm->payload_pos = 0;

        LOG_D(TAG, "Function: %d", c);

        switch (c)
        {
        case LTM_GFRAME:
            ltm->length = LTM_GFRAME_SIZE;
            break;
        case LTM_AFRAME:
            ltm->length = LTM_AFRAME_SIZE;
            break;
        case LTM_SFRAME:
            ltm->length = LTM_SFRAME_SIZE;
            break;
        case LTM_OFRAME:
            ltm->length = LTM_OFRAME_SIZE;
            break;
        case LTM_NFRAME:
            ltm->length = LTM_NFRAME_SIZE;
            break;
        case LTM_XFRAME:
            ltm->length = LTM_XFRAME_SIZE;
            break;
        default:
            ltm->status = LTM_IDLE;
            break;
        }
    }
    else if (ltm->status == LTM_STATE_MSGTYPE)
    {
        ltm->status = LTM_STATE_DATA;
        ltm->payload[ltm->payload_pos++] = c;
        ltm->crc = c;
    }
    else if (ltm->status == LTM_STATE_DATA)
    {
        if (ltm->payload_pos < (ltm->length - 4))
        {
            ltm->payload[ltm->payload_pos++] = c;
            ltm->crc ^= c;
        }
        else 
        {
            if (ltm->crc == c)
            {
                switch (ltm->function)
                {
                case LTM_GFRAME:
                    memcpy(ltm->gframe, &ltm->payload, sizeof(ltm_gframe_t));
                    break;
                case LTM_AFRAME:
                    memcpy(ltm->aframe, &ltm->payload, sizeof(ltm_aframe_t));
                    break;
                case LTM_SFRAME:
                    memcpy(ltm->sframe, &ltm->payload, sizeof(ltm_sframe_t));
                    break;
                case LTM_OFRAME:
                    memcpy(ltm->oframe, &ltm->payload, sizeof(ltm_oframe_t));
                    break;
                case LTM_NFRAME:
                    memcpy(ltm->nframe, &ltm->payload, sizeof(ltm_nframe_t));
                    break;
                case LTM_XFRAME:
                    memcpy(ltm->xframe, &ltm->payload, sizeof(ltm_xframe_t));
                    break;
                }

                ret = true;
            }

            ltm->status = LTM_IDLE;
        }
    }
    else
    {
        ltm->status = LTM_IDLE; 
    }
    

    return ret;
}

void ltm_destroy(ltm_t *ltm)
{
    free(ltm->io);
}