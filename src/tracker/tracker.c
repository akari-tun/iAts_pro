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
        .max_steps = DEFAULT_EASE_MAX_STEPS,
        .max_ms = DEFAULT_EASE_MAX_MS,
	    .min_ms = DEFAULT_EASE_MIN_MS,
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
    
    servo_init(&servo);
    atp_init(&atp);
}

void task_tracker(void *arg)
{
    tracker_t *t = arg;

    // time_ticks_t sleep;
    time_millis_t now;
    time_millis_t pan_tick = 0;
    time_millis_t tilt_tick = 0;

    // servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.max_pulsewidth);
    // servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.max_pulsewidth);
    servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.min_pulsewidth);
    servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.min_pulsewidth);

    vTaskDelay(MILLIS_TO_TICKS(3000));

    uint16_t distance = 0;

    while (1)
    {
        now = time_millis_now();

        if (t->internal.mode == TRACKER_MODE_AUTO)
        {
            if (t->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED))
            {
                //pan
                if (now > pan_tick)
                {
                    if (servo.internal.pan.is_easing)
                    {
                        pan_tick = now + servo_get_easing_sleep(&servo.internal.pan);
                        servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);

                        // LOG_I(TAG, "step_to:%d | step_positon:%d | step_sleep_ms:%d\n", 
                        //     servo.internal.pan.step_to, servo.internal.pan.step_positon, servo.internal.pan.step_sleep_ms);
                    }
                    else
                    {
                        float tracker_lat = telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_LATITUDE)) / 10000000.0f;
                        float tracker_lon = telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_LONGITUDE)) / 10000000.0f;
                        float plane_lat = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LATITUDE)) / 10000000.0f;
                        float plane_lon = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LONGITUDE)) / 10000000.0f;

                        distance = distance_between(tracker_lat, tracker_lon, plane_lat, plane_lon);

                        // LOG_D(TAG, "t_lat:%f | t_lon:%f | p_lat:%f | p_lon:%f | dist:%d\n", tracker_lat, tracker_lon, plane_lat, plane_lon, distance);

                        uint16_t course_deg = course_to(tracker_lat, tracker_lon, plane_lat, plane_lon);
                        course_deg = course_deg + servo.internal.course;
                        if (course_deg >= 360u)
                        {
                            course_deg = course_deg - 360u;
                        }
                            
                        if (course_deg != servo.internal.pan.currtent_degree)
                        {
                            servo.internal.pan.currtent_degree = course_deg;
                            servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);
                        }

                        pan_tick = now + (servo.internal.pan.is_easing ? servo_get_easing_sleep(&servo.internal.pan) : 100);
                    }
                }

                //tilt
                if (now > tilt_tick)
                {
                    if (servo.internal.tilt.is_easing)
                    {
                        tilt_tick = now + servo_get_easing_sleep(&servo.internal.tilt);
                        servo_pulsewidth_control(&servo.internal.tilt, &servo.internal.ease_config);

                        // LOG_I(TAG, "step_to:%d | step_positon:%d | step_sleep_ms:%d\n", 
                        //     servo.internal.tilt.step_to, servo.internal.tilt.step_positon, servo.internal.tilt.step_sleep_ms);
                    }
                    else
                    {
                        int32_t tracker_alt = telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_ALTITUDE));
                        int32_t plane_alt = telemetry_get_i32(atp_get_tag_val(TAG_PLANE_ALTITUDE));

                        // LOG_D(TAG, "t_alt:%d | p_alt:%d | dist:%d\n", tracker_alt, plane_alt, distance);

                        uint16_t tilt_deg = tilt_to(distance, tracker_alt, plane_alt);

                        if (tilt_deg != servo.internal.tilt.currtent_degree || servo.internal.tilt.is_reverse != servo.internal.pan.is_reverse)
                        {
                            servo.internal.tilt.is_reverse = servo.internal.pan.is_reverse;
                            servo.internal.tilt.currtent_degree = tilt_deg;
                            servo_pulsewidth_control(&servo.internal.tilt, &servo.internal.ease_config);
                        }

                        tilt_tick = now + (servo.internal.tilt.is_easing ? servo_get_easing_sleep(&servo.internal.tilt) : 100);
                    }
                }

                servo_reverse_check(&servo);
            }
        }
        else if (t->internal.mode == TRACKER_MODE_MANUAL)
        {
            servo.internal.tilt.is_reverse = servo.internal.pan.is_reverse;
            servo_update(&servo);
        }

        // if (servo.internal.tilt.is_easing || servo.internal.pan.is_easing)
        // {
        //     sleep = MILLIS_TO_TICKS(10);
        //     servo_update(&servo);
        // }
        // else
        // {
        //     sleep = MILLIS_TO_TICKS(100);
        // }

        if (!tracker_check_atp_cmd(t))
        {
            vTaskDelay(MILLIS_TO_TICKS(1));
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