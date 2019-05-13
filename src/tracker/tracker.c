#include <hal/log.h>
#include "tracker.h"
#include "protocols/atp.h"
#include "util/calc.h"

static const char *TAG = "Tarcker";
static servo_t servo;
static atp_t atp;

// static Observer telemetry_vals_observer;

static void tracker_status_changed(void *t, tracker_status_e s)
{
    LOG_I(TAG, "TRACKER_STATUS_CHANGE -> %d", s);

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
    LOG_I(TAG, "TRACKER_FLAG_CHANGED -> %d | %d = %d", tracker->internal.flag, f, tracker->internal.flag | f);

    tracker->internal.flag |= f;
    time_micros_t now = time_micros_now();

    // #if defined(USE_BEEPER)
    //     if (f & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED)) beeper_set_mode(&t->ui.internal.beeper, BEEPER_MODE_SETED);
    // #endif
    
    ATP_SET_U8(TAG_TRACKER_FLAG, tracker->internal.flag, now);
    tracker->internal.flag_changed_notifier->mSubject.Notify(tracker->internal.flag_changed_notifier, &f);
}

static void tracker_telemetry_changed(void *t, uint8_t tag)
{
    tracker_t *tracker = (tracker_t *)t;

    switch (tag)
    {
    case TAG_BASE_ACK:
        if (!(telemetry_get_u8(atp_get_tag_val(TAG_TRACKER_FLAG)) & TRACKER_FLAG_SERVER_CONNECTED))
        {   
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_SERVER_CONNECTED);
        }
        tracker->last_heartbeat = time_millis_now();
        break;
    case TAG_PLANE_LONGITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_PLANESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_PLANESETED);
        break;
    case TAG_PLANE_LATITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_PLANESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_PLANESETED);
        break;
    case TAG_TRACKER_LONGITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_HOMESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_HOMESETED);
    case TAG_TRACKER_LATITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_HOMESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_HOMESETED);
        break;
    case TAG_TRACKER_MODE:
        tracker->internal.status = telemetry_get_u8(atp_get_tag_val(tag));
        break;
    case TAG_TRACKER_FLAG:
        break;
    default:
        break;
    }
}

static bool tracker_check_atp_cmd(tracker_t *t)
{
    if (!(t->internal.flag & TRACKER_FLAG_WIFI_CONNECTED))
        return false;

    time_millis_t now = time_millis_now(); 

    // if (t->last_heartbeat + 30000 < now && t->internal.flag & TRACKER_FLAG_SERVER_CONNECTED)
    // {
    //     t->internal.status_changed(t, TRACKER_STATUS_SERVER_CONNECTING);
    //     return false;
    // }

    if (!(t->internal.flag & TRACKER_FLAG_SERVER_CONNECTED))
    {
        
        if (now > t->last_heartbeat + 2000)
        {
            t->atp->enc_frame->atp_cmd = CMD_HEARTBEAT;
            uint8_t *buff = atp_frame_encode(t->atp->enc_frame);
            if (t->atp->enc_frame->buffer_index > 0)
            {
                t->atp->atp_send(buff, t->atp->enc_frame->buffer_index);
                t->last_heartbeat = now;
                return true;
            }
        }
    }
    else
    {
        if (now > t->last_heartbeat + 10000)
        {
            t->atp->enc_frame->atp_cmd = CMD_HEARTBEAT;
            uint8_t *buff = atp_frame_encode(t->atp->enc_frame);
            if (t->atp->enc_frame->buffer_index > 0)
            {
                t->atp->atp_send(buff, t->atp->enc_frame->buffer_index);
                t->last_heartbeat = now;
                return true;
            }
        }
    }

    return false;
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
    t->internal.telemetry_changed = tracker_telemetry_changed;
    t->internal.status_changed(t, TRACKER_STATUS_BOOTING);
    t->last_heartbeat = time_millis_now();

    t->servo = &servo;
    t->atp = &atp;
    t->atp->tracker = t;
    t->atp->tag_value_changed = t->internal.telemetry_changed;
    
    // atp.flag_change = tracker_flag_changed;
    // atp.telemetry_val_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));
    // telemetry_vals_observer.Obj = t;
    // telemetry_vals_observer.Name = "Telemetry values observer";
    // telemetry_vals_observer.Update = telemetry_vals_updated;
    // atp.telemetry_val_notifier->mSubject.Attach(atp.telemetry_val_notifier, &telemetry_vals_observer);

    servo_init(&servo);
    atp_init(&atp);
}

void task_tracker(void *arg)
{
    tracker_t *t = arg;

    time_ticks_t sleep;

    // servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.max_pulsewidth);
    // servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.max_pulsewidth);
    servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.min_pulsewidth);
    servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.min_pulsewidth);

    vTaskDelay(MILLIS_TO_TICKS(3000));

    uint16_t distance = 0;

    while (1)
    {
        if (t->internal.mode == TRACKER_MODE_AUTO)
        {
            if (t->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED))
            {
                //pan
                if (TIME_CYCLE_EVERY_MS(150, 2) == 0)
                {
                    float tracker_lat = telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_LATITUDE)) / 10000000.0f;
                    float tracker_lon = telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_LONGITUDE)) / 10000000.0f;
                    float plane_lat = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LATITUDE)) / 10000000.0f;
                    float plane_lon = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LONGITUDE)) / 10000000.0f;

                    distance = distance_between(tracker_lat, tracker_lon, plane_lat, plane_lon);
                    // printf("tracker_lat:%f | tracker_lon:%f | plane_lat:%f | plane_lon:%f | distance:%d\n", tracker_lat, tracker_lon, plane_lat, plane_lon, distance);
                    servo.internal.pan.currtent_degree = course_to(tracker_lat, tracker_lon, plane_lat, plane_lon);
                    servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);
                }

                //tilt
                if (TIME_CYCLE_EVERY_MS(200, 2) == 0)
                {
                    int32_t tracker_alt = telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_ALTITUDE));
                    int32_t plane_alt = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_ALTITUDE));

                    // printf("tracker_alt:%d | plane_alt:%d | distance:%d\n", tracker_alt, plane_alt, distance);
                    servo.internal.tilt.currtent_degree = tilt_to(distance, tracker_alt, plane_alt);
                    servo_pulsewidth_control(&servo.internal.tilt, &servo.internal.ease_config);
                }

                servo_reverse_check(&servo);
            }
        }
        else if (t->internal.mode == TRACKER_MODE_MANUAL)
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

        if (!tracker_check_atp_cmd(t))
        {
            vTaskDelay(sleep);
        }
    }
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