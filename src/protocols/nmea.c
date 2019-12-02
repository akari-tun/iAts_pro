#include "nmea.h"

#include <hal/log.h>
#include "atp.h"

static gps_t gps;

static const char *TAG = "Protocol.Nmea";

void nmea_init(nmea_t *nmea)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    nmea->io = (io_t *)malloc(sizeof(io_t));;
    nmea->gps = &gps;
    nmea->buf_pos = 0;

    memset(&nmea->buf, 0, sizeof(nmea->buf));

    LOG_I(TAG, "Initialized");
}

int nmea_update(nmea_t *nmea, void *data)
{
    int rem = sizeof(nmea->buf)  - nmea->buf_pos;
    int n = io_read(nmea->io, &nmea->buf[nmea->buf_pos], rem, 0);

    uint8_t ret = 0;

    if (n <= 0 && nmea->buf_pos == 0)
    {
        return ret;
    }

    nmea->buf_pos += n;
    LOG_D(TAG, "Read %d bytes, buf at %u", n, nmea->buf_pos);

    if (gps_process(&gps, &nmea->buf, nmea->buf_pos))
    {
        if (gps.fix_mode >= 3 && (telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)) != (int32_t)(gps.longitude * 10000000.0f) || telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)) != (int32_t)(gps.latitude * 10000000.0f)))
        {
            time_micros_t now = time_micros_now();
            atp_t *atp = (atp_t *)data;

            ATP_SET_I32(TAG_PLANE_LONGITUDE, (int32_t)(gps.longitude * 10000000.0f), now);
            ATP_SET_I32(TAG_PLANE_LATITUDE, (int32_t)(gps.latitude * 10000000.0f), now);
            ATP_SET_I32(TAG_PLANE_ALTITUDE, (int32_t)(gps.altitude * 100), now);
            ATP_SET_I16(TAG_PLANE_SPEED, (int16_t)gps_to_speed(gps.speed, gps_speed_mps), now);
            ATP_SET_U16(TAG_PLANE_HEADING, (uint16_t)gps.coarse, now);
            atp->tag_value_changed(atp->tracker, TAG_PLANE_LATITUDE);
            atp->tag_value_changed(atp->tracker, TAG_PLANE_LONGITUDE);

            ret = 1;
        }
    }

    nmea->buf_pos = 0;

    return ret;
}

void nmea_destroy(nmea_t *nmea)
{
    free(nmea->io);
    free(nmea->gps);
}