#include "config/settings.h"
#include "protocols/atp.h"
#include "home.h"

void home_init(home_t *home)
{
    //home = (home_t *)malloc(sizeof(home_t));
    home->real_time = settings_get_key_bool(SETTING_KEY_HOME_REAL_TIME);
    home->seted = false;
}

void home_update(home_t *home)
{
    if (home->real_time || !home->seted)
    {
        home->latitude = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE)) /  10000000.0f;
        home->longitude = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE)) /  10000000.0f;
        home->altitude = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE));
    }
}

void home_save(home_t *home)
{
    setting_set_i32(settings_get_key(SETTING_KEY_HOME_LAT), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE)));
    setting_set_i32(settings_get_key(SETTING_KEY_HOME_LON), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE)));
    setting_set_i32(settings_get_key(SETTING_KEY_HOME_ALT), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE)));
}