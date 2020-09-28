#include "mavlink.h"

#include <hal/log.h>
#include "../lib/c_library_v2/common/mavlink.h"
#include "atp.h"

static mavlink_status_t mavlink_status;
static mavlink_message_t mavlink_message;

static const char *TAG = "Protocol.Mavlink";

void mavlink_init(mavlink_t *mavlink)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    mavlink->io = (io_t *)malloc(sizeof(io_t));;
    mavlink->buf_pos = 0;
    mavlink->status = &mavlink_status;
    mavlink->message = &mavlink_message;

    mavlink->message_value.global_position = (mavlink_global_position_int_t *)malloc(sizeof(mavlink_global_position_int_t));
    mavlink->message_value.home_position = (mavlink_home_position_t *)malloc(sizeof(mavlink_home_position_t));

    memset(&mavlink->buf, 0, sizeof(mavlink->buf));

    LOG_I(TAG, "Initialized");
}

int mavlink_update(mavlink_t *mavlink, void *data)
{
    int chan = MAVLINK_COMM_0;
    int rem = sizeof(mavlink->buf)  - mavlink->buf_pos;
    int n = io_read(mavlink->io, &mavlink->buf[mavlink->buf_pos], rem, 0);

    uint8_t ret = 0;

    if (n <= 0 && mavlink->buf_pos == 0)
    {
        return ret;
    }

    mavlink->buf_pos += n;
    LOG_D(TAG, "Read %d bytes, buf at %u", n, mavlink->buf_pos);

    int start = 0;
    int end = mavlink->buf_pos;
    ret = 1;

    while(start < mavlink->buf_pos)
    {
        if (mavlink_parse_char(chan, mavlink->buf[start], mavlink->message, mavlink->status))
        {
            LOG_D(TAG, "Received message with ID [%d], sequence: [%d] from component [%d] of system [%d]", mavlink->message->msgid, mavlink->message->seq, mavlink->message->compid, mavlink->message->sysid);

            mavlink->counter++;

            if (mavlink->message->seq == 0xff)
            {
                mavlink->link_quality = mavlink->counter / 255.0f;
                mavlink->counter = 0;
                LOG_I(TAG, "link_quality: [%.2f] ", mavlink->link_quality);
            }

            time_micros_t now = time_micros_now();
            atp_t *atp = (atp_t *)data;

            switch(mavlink->message->msgid) 
            {
            case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: // ID for GLOBAL_POSITION_INT
                // Get all fields in payload (into global_position)
                mavlink_msg_global_position_int_decode(mavlink->message, mavlink->message_value.global_position);
                ATP_SET_I32(TAG_PLANE_LONGITUDE,  mavlink->message_value.global_position->lon, now);
                ATP_SET_I32(TAG_PLANE_LATITUDE, mavlink->message_value.global_position->lat, now);
                ATP_SET_I32(TAG_PLANE_ALTITUDE, mavlink->message_value.global_position->alt / 10, now);
                atp->tag_value_changed(atp->tracker, TAG_PLANE_LATITUDE);
                atp->tag_value_changed(atp->tracker, TAG_PLANE_LONGITUDE);
                break;
            case MAVLINK_MSG_ID_HOME_POSITION:
                mavlink_msg_home_position_decode(mavlink->message, mavlink->message_value.home_position);
                ATP_SET_I32(TAG_TRACKER_LONGITUDE,  mavlink->message_value.home_position->longitude, now);
                ATP_SET_I32(TAG_TRACKER_LATITUDE,  mavlink->message_value.home_position->latitude, now);
                ATP_SET_I32(TAG_TRACKER_ALTITUDE,  mavlink->message_value.home_position->altitude / 10, now);
                atp->tag_value_changed(atp->tracker, TAG_TRACKER_LONGITUDE);
                atp->tag_value_changed(atp->tracker, TAG_TRACKER_LATITUDE);
                atp->tag_value_changed(atp->tracker, TAG_TRACKER_ALTITUDE);
                break;
            case MAVLINK_MSG_ID_GPS_RAW_INT:
                //mavlink_msg_gps_raw_int_decode(mavlink->message, mavlink->message_value.gps_raw);
                //ATP_SET_I16(TAG_PLANE_SPEED,  (uint16_t)(mavlink->message_value.gps_raw->vel / 100), now);
                break;
            }

            ret = 2;
            break;
        }

        start++;
    }

    if (start > 0)
    {
        LOG_D(TAG, "Consumed %d bytes", start);
        LOG_BUFFER_D(TAG, &mavlink->buf[0], start);
        if (start != mavlink->buf_pos)
        {
            // Got some data at the end that we need to copy
            memmove(mavlink->buf, &mavlink->buf[start], end - start);
        }
        mavlink->buf_pos -= start;
    }

    return ret;
}

bool mavlink_has_buffered_data(mavlink_t *mavlink)
{
    return mavlink->buf_pos > 0 && mavlink->buf_pos < sizeof(mavlink->buf);
}

void mavlink_port_reset(mavlink_t *mavlink)
{
    mavlink->buf_pos = 0;
}

void mavlink_destroy(mavlink_t *mavlink)
{
    free(mavlink->io);
    free(mavlink->message_value.global_position);
    free(mavlink->message_value.home_position);
}