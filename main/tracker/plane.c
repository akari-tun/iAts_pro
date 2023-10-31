#include "config/settings.h"
#include "protocols/atp.h"
#include "plane.h"

void plane_init(plane_t *plane)
{
    //plane = (plane_t *)malloc(sizeof(plane_t));
    plane->seted = false;
}

void plane_update(plane_t *plane)
{
    plane->latitude = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)) /  10000000.0f;
    plane->longitude = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)) /  10000000.0f;
    plane->altitude = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE));
}