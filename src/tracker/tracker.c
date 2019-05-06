#include <hal/log.h>
#include "tracker.h"
#include "protocols/atp.h"
#include "util/calc.h"

// static const char *TAG = "Tarcker";
static servo_t servo;

static void tracker_status_changed(void *t, tracker_status_e s)
{
    tracker_t *tracker = t;
    tracker->internal.status = s;

    switch (tracker->internal.status)
    {
    case TRACKER_STATUS_BOOTING:
        break;
    case TRACKER_STATUS_WIFI_SMART_CONFIG:
        break;
    case TRACKER_STATUS_WIFI_CONNECTING:
        break;
    case TRACKER_STATUS_WIFI_CONNECTED:
        // tracker->ui->internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
        break;
    case TRACKER_STATUS_SERVER_CONNECTING:
        break;
    case TRACKER_STATUS_TRACKING:
        break;
    case TRACKER_STATUS_MANUAL:
        break;
    }

    tracker->internal.status_changed_notifier->mSubject.Notify(tracker->internal.status_changed_notifier, &s);
}

static void tracker_flag_changed(void *t, uint8_t f)
{
    tracker_t *tracker = t;
    tracker->internal.flag |= f;

    // #if defined(USE_BEEPER)
    //     if (f & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED)) beeper_set_mode(&t->ui.internal.beeper, BEEPER_MODE_SETED);
    // #endif

    tracker->internal.flag_changed_notifier->mSubject.Notify(tracker->internal.flag_changed_notifier, &f);
}

void tracker_init(tracker_t *t)
{
    ease_config_t e_cfg = {
        .steps = DEFAULT_EASE_STEPS,
        .min_pulsewidth = DEFAULT_EASE_MIN_PULSEWIDTH,
        .ease_out = DEFAULT_EASE_OUT,
    };

    servo.internal.ease_config = e_cfg;

    t->internal.mode = TRACKER_MODE_AUTO;
    t->internal.flag_changed_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));
    t->internal.status_changed_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));
    t->internal.status_changed = tracker_status_changed;
    t->internal.flag_changed = tracker_flag_changed;
    t->internal.status_changed(t, TRACKER_STATUS_BOOTING);
    t->servo = &servo;

    // servo_init(&servo, t->ui);
    servo_init(&servo);
    atp_init(t);
}

void task_tracker(void *arg)
{
    tracker_t *tracker = arg;

    time_ticks_t sleep;

    servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.max_pulsewidth);
    servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.max_pulsewidth);
    vTaskDelay(MILLIS_TO_TICKS(3000));
    servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.min_pulsewidth);
    servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.min_pulsewidth);

    uint16_t distance = 0;

    while (1)
    {
        if (tracker->internal.mode == TRACKER_MODE_AUTO)
        {
            if (tracker->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED))
            {
                //pan
                if (TIME_CYCLE_EVERY_MS(150, 2) == 0)
                {
                    float tracker_lat = telemetry_get_i32(atp_get_tag_val(TAG_HOME_LATITUDE)) / 10000000.0f;
                    float tracker_lon = telemetry_get_i32(atp_get_tag_val(TAG_HOME_LONGITUDE)) / 10000000.0f;
                    float plane_lat = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LATITUDE)) / 10000000.0f;
                    float plane_lon = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LONGITUDE)) / 10000000.0f;

                    distance = distance_between(tracker_lat, tracker_lon, plane_lat, plane_lon);

                    servo.internal.pan.currtent_degree = course_to(tracker_lat, tracker_lon, plane_lat, plane_lon) * 10.0f;
                    servo.internal.pan.is_reverse = servo.internal.pan.currtent_degree > 180;
                    servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);
                    // tracker->ui->internal.led_gradual_target.pan = servo.internal.pan.currtent_degree;
                    // tracker->ui->internal.led_gradual_target.pan_pulsewidth = servo.internal.pan.currtent_pulsewidth;
                }

                //tilt
                if (TIME_CYCLE_EVERY_MS(200, 2) == 0)
                {
                    int16_t tracker_alt = telemetry_get_i16(atp_get_tag_val(TAG_HOME_ALTITUDE));
                    int16_t plane_alt = telemetry_get_i16(atp_get_tag_val(TAG_HOME_ALTITUDE));

                    servo.internal.tilt.currtent_degree = tilt_to(distance, tracker_alt, plane_alt);
                    servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);
                    // tracker->ui->internal.led_gradual_target.pan = servo.internal.pan.currtent_degree;
                    // tracker->ui->internal.led_gradual_target.pan_pulsewidth = servo.internal.pan.currtent_pulsewidth;
                }
            }
        }
        else if (tracker->internal.mode == TRACKER_MODE_MANUAL)
        {
            servo_update(&servo);
        }

        if (servo.internal.tilt.is_easing || servo.internal.pan.is_easing)
        {
            sleep = MILLIS_TO_TICKS(10);
            servo_update(&servo);
        }
        else
        {
            sleep = MILLIS_TO_TICKS(50);
        }

        vTaskDelay(sleep);
    }
}

const char *telemetry_format_tracker_mode(const telemetry_t *val, char *buf, size_t bufsize)
{
    switch ((tracker_mode_e)val->val.u8)
    {
    case TRACKER_MODE_AUTO:
        return "Tracking";
    case TRACKER_MODE_MANUAL:
        return "Manual";
    case TRACKER_MODE_DEBUG:
        return "Debug";
    }
    return NULL;
}

tracker_status_e get_tracker_status(const tracker_t *t)
{
    return t->internal.status;
}

uint8_t get_tracker_flag(const tracker_t *t)
{
    return t->internal.flag;
}

bool get_tracker_reversing(const tracker_t *t)
{
    return t->servo->is_reversing;
}