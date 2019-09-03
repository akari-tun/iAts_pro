#include <hal/log.h>
#include <hal/wd.h>

#include "config/settings.h"
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
        tracker->internal.status_changed(tracker, TRACKER_STATUS_TRACKING);
        break;
    case TRACKER_STATUS_TRACKING:
        break;
    case TRACKER_STATUS_MANUAL:
        break;
    }

    tracker->internal.status_changed_notifier->mSubject.Notify(tracker->internal.status_changed_notifier, &s);
}

static void tracker_flag_changed(void *t, uint8_t f, uint8_t v)
{
    tracker_t *tracker = t;
    uint8_t flag = tracker->internal.flag;

    if (v > 0)
    {
        tracker->internal.flag |= f;
    }
    else 
    {
        tracker->internal.flag &= ~f;
    }
    
    LOG_I(TAG, "TRACKER_FLAG_CHANGED -> %d set %d to %d = %d", flag, f, v, tracker->internal.flag);

    time_micros_t now = time_micros_now();

    ATP_SET_U8(TAG_TRACKER_FLAG, tracker->internal.flag, now);
    tracker->internal.flag_changed_notifier->mSubject.Notify(tracker->internal.flag_changed_notifier, &f);
}

static void tracker_telemetry_changed(void *t, uint8_t tag)
{
    tracker_t *tracker = (tracker_t *)t;

    switch (tag)
    {
    case TAG_BASE_ACK:
        if (!(telemetry_get_u8(atp_get_telemetry_tag_val(TAG_TRACKER_FLAG)) & TRACKER_FLAG_SERVER_CONNECTED))
        {
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_SERVER_CONNECTED, 1);
        }
        tracker->last_ack = time_millis_now();
        break;
    case TAG_PLANE_LONGITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_PLANESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_PLANESETED, 1);
        break;
    case TAG_PLANE_LATITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_PLANESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_PLANESETED, 1);
        break;
    case TAG_TRACKER_LONGITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_HOMESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_HOMESETED, 1);
    case TAG_TRACKER_LATITUDE:
        if (!(tracker->internal.flag & TRACKER_FLAG_HOMESETED))
            tracker->internal.flag_changed(tracker, TRACKER_FLAG_HOMESETED, 1);
        break;
    case TAG_TRACKER_MODE:
        tracker->internal.status = telemetry_get_u8(atp_get_telemetry_tag_val(tag));
        break;
    case TAG_TRACKER_FLAG:
        break;
    default:
        break;
    }
}

static void tracker_settings_handler(const setting_t *setting, void *user_data)
{
    tracker_t *t = (tracker_t *)user_data;

    if (SETTING_IS(setting, SETTING_KEY_SERVO_COURSE))
    {
        t->servo->internal.course = setting_get_u16(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_DIRECTION))
    {
        t->servo->internal.pan.config.direction = setting_get_u8(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_TILT_DIRECTION))
    {
        t->servo->internal.tilt.config.direction = setting_get_u8(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH))
    {
        t->servo->internal.pan.config.max_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.pan.currtent_pulsewidth > t->servo->internal.pan.config.max_pulsewidth)
            t->servo->internal.pan.currtent_pulsewidth = t->servo->internal.pan.config.max_pulsewidth;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH))
    {
        t->servo->internal.pan.config.min_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.pan.currtent_pulsewidth < t->servo->internal.pan.config.min_pulsewidth)
            t->servo->internal.pan.currtent_pulsewidth = t->servo->internal.pan.config.min_pulsewidth;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH))
    {
        t->servo->internal.tilt.config.max_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.tilt.currtent_pulsewidth > t->servo->internal.tilt.config.max_pulsewidth)
            t->servo->internal.tilt.currtent_pulsewidth = t->servo->internal.tilt.config.max_pulsewidth;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH))
    {
        t->servo->internal.tilt.config.min_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.tilt.currtent_pulsewidth < t->servo->internal.tilt.config.min_pulsewidth)
            t->servo->internal.tilt.currtent_pulsewidth = t->servo->internal.tilt.config.min_pulsewidth;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_OUT_TYPE))
    {
        t->servo->internal.ease_config.ease_out = setting_get_u8(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MAX_STEPS))
    {
        t->servo->internal.ease_config.max_steps = setting_get_u16(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MIN_PULSEWIDTH))
    {
        t->servo->internal.ease_config.min_pulsewidth = setting_get_u16(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_STEP_MS))
    {
        t->servo->internal.ease_config.step_ms = setting_get_u16(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MAX_MS))
    {
        t->servo->internal.ease_config.max_ms = setting_get_u16(setting);
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MIN_MS))
    {
        t->servo->internal.ease_config.min_ms = setting_get_u16(setting);
    }
}

static bool tracker_check_atp_cmd(tracker_t *t)
{
    if (!(t->internal.flag & TRACKER_FLAG_WIFI_CONNECTED))
        return false;

    time_millis_t now = time_millis_now();

    if (!(t->internal.flag & TRACKER_FLAG_SERVER_CONNECTED))
    {
        if (now > t->last_heartbeat + 1000)
        {
            //printf("!TRACKER_FLAG_SERVER_CONNECTED\n");
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
        if (now > t->last_heartbeat + 5000)
        {
            //printf("TRTACKER_FLAG_SERVER_CONNECTED\n");
            t->atp->enc_frame->atp_cmd = CMD_HEARTBEAT;
            uint8_t *buff = atp_frame_encode(t->atp->enc_frame);
            if (t->atp->enc_frame->buffer_index > 0)
            {
                t->atp->atp_send(buff, t->atp->enc_frame->buffer_index);
                t->last_heartbeat = now;
                return true;
            }
        }
        else if (t->atp->atp_cmd->cmds[0] > 0)
        {
            t->atp->enc_frame->atp_cmd = atp_popup_cmd();;
            uint8_t *buff = atp_frame_encode(t->atp->enc_frame);
            if (t->atp->enc_frame->buffer_index > 0)
            {
                t->atp->atp_send(buff, t->atp->enc_frame->buffer_index);
                return true;
            }
        }
    }

    return false;
}

static bool tracker_check_atp_ctr(tracker_t *t)
{
    const setting_t *setting;

    if (t->atp->atp_ctr->ctrs[0] > 0)
    {
        switch (t->atp->atp_ctr->ctrs[0])
        {
        case TAG_CTR_MODE:
            break;
        case TAG_CTR_AUTO_POINT_TO_NORTH:
            break;
        case TAG_CTR_CALIBRATE:
            break;
        case TAG_CTR_HEADING:
            break;
        case TAG_CTR_TILT:
            break;
        case TAG_CTR_REBOOT:
            LOG_I(TAG, "Execute [REBOOT]");

            setting = settings_get_key(SETTING_KEY_DEVELOPER_REBOOT);
            setting_set_u8(setting, t->atp->atp_ctr->data[0]);
            atp_remove_ctr(sizeof(uint8_t));
        case TAG_CTR_SMART_CONFIG:
            LOG_D(TAG, "Execute [SMART_CONFIG]");

            setting = settings_get_key(SETTING_KEY_WIFI_SMART_CONFIG);
            setting_set_u8(setting, t->atp->atp_ctr->data[0]);
            atp_remove_ctr(sizeof(uint8_t));
            break;
        }
    }

    return true;
}

void tracker_init(tracker_t *t)
{
    // ease_config_t e_cfg = {
    //     .max_steps = DEFAULT_EASE_MAX_STEPS,
    //     .max_ms = DEFAULT_EASE_MAX_MS,
    //     .min_ms = DEFAULT_EASE_MIN_MS,
    //     .min_pulsewidth = DEFAULT_EASE_MIN_PULSEWIDTH,
    //     .ease_out = DEFAULT_EASE_OUT_TYPE,
    // };

    esp_log_level_set(TAG, ESP_LOG_INFO);

    ease_config_t e_cfg = {
        .max_steps = settings_get_key_u16(SETTING_KEY_SERVO_EASE_MAX_STEPS),
        .max_ms = settings_get_key_u16(SETTING_KEY_SERVO_EASE_MAX_MS),
        .min_ms = settings_get_key_u16(SETTING_KEY_SERVO_EASE_MIN_MS),
        .min_pulsewidth = settings_get_key_u16(SETTING_KEY_SERVO_EASE_MIN_PULSEWIDTH),
        .step_ms = settings_get_key_u16(SETTING_KEY_SERVO_EASE_STEP_MS),
        .ease_out = settings_get_key_u8(SETTING_KEY_SERVO_EASE_OUT_TYPE),
    };

    servo.internal.ease_config = e_cfg;

    // t->internal.mode = TRACKER_MODE_AUTO;
    t->internal.show_coordinate = settings_get_key_bool(SETTING_KEY_TRACKER_SHOW_COORDINATE),
    t->internal.flag_changed_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));
    t->internal.status_changed_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));
    t->internal.status_changed = tracker_status_changed;
    t->internal.flag_changed = tracker_flag_changed;
    t->internal.telemetry_changed = tracker_telemetry_changed;
    t->internal.status_changed(t, TRACKER_STATUS_BOOTING);
    t->last_heartbeat = time_millis_now();
    t->last_ack = time_millis_now();

    t->servo = &servo;
    t->atp = &atp;
    t->atp->tracker = t;
    t->atp->tag_value_changed = t->internal.telemetry_changed;

    settings_add_listener(tracker_settings_handler, t);

    servo_init(&servo);
    atp_init(&atp);
}

void task_tracker(void *arg)
{
    tracker_t *t = arg;

    time_millis_t now;

    servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.min_pulsewidth);
    servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.min_pulsewidth);

    vTaskDelay(MILLIS_TO_TICKS(1000));

    hal_wd_add_task(NULL);

    uint16_t distance = 0;

    while (1)
    {
        now = time_millis_now();

        //pan
        if (now > servo.internal.pan.next_tick)
        {
            if (servo.internal.pan.is_easing)
            {
                servo.internal.pan.next_tick = now + servo_get_easing_sleep(&servo.internal.pan);
                servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);
                // LOG_D(TAG, "[pan] positon:%d -> to:%d | sleep:%dms | pwm:%d\n", servo.internal.pan.step_positon, servo.internal.pan.step_to, servo.internal.pan.step_sleep_ms, servo.internal.pan.last_pulsewidth);
            }
            else
            {
                if (t->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED) && t->internal.status == TRACKER_STATUS_TRACKING)
                {
                    float tracker_lat = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE)) / 10000000.0f;
                    float tracker_lon = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE)) / 10000000.0f;
                    float plane_lat = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)) / 10000000.0f;
                    float plane_lon = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)) / 10000000.0f;

                    distance = distance_between(tracker_lat, tracker_lon, plane_lat, plane_lon);

                    //LOG_D(TAG, "t_lat:%f | t_lon:%f | p_lat:%f | p_lon:%f | dist:%d\n", tracker_lat, tracker_lon, plane_lat, plane_lon, distance);

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
                }

                servo.internal.pan.next_tick = now + (servo.internal.pan.is_easing ? servo_get_easing_sleep(&servo.internal.pan) : 50);
            }
        }

        //tilt
        if (now > servo.internal.tilt.next_tick)
        {
            if (servo.internal.tilt.is_easing)
            {
                servo.internal.tilt.next_tick = now + servo_get_easing_sleep(&servo.internal.tilt);
                servo_pulsewidth_control(&servo.internal.tilt, &servo.internal.ease_config);
                //LOG_D(TAG, "[tilt] positon:%d -> to:%d | sleep:%dms | pwm:%d\n", servo.internal.tilt.step_positon, servo.internal.tilt.step_to, servo.internal.tilt.step_sleep_ms, servo.internal.tilt.last_pulsewidth );
            }
            else
            {
                if (t->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED) && t->internal.status == TRACKER_STATUS_TRACKING)
                {
                    int32_t tracker_alt = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE));
                    int32_t plane_alt = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE));

                    uint16_t tilt_deg = tilt_to(distance, tracker_alt, plane_alt);

                    //LOG_D(TAG, "t_alt:%d | p_alt:%d | dist:%d | tilt_deg:%d\n", tracker_alt, plane_alt, distance, tilt_deg);

                    if (tilt_deg != servo.internal.tilt.currtent_degree || servo.internal.tilt.is_reverse != servo.internal.pan.is_reverse)
                    {
                        servo.internal.tilt.currtent_degree = tilt_deg;
                        servo_pulsewidth_control(&servo.internal.tilt, &servo.internal.ease_config);
                    }
                }

                servo.internal.tilt.next_tick = now + (servo.internal.tilt.is_easing ? servo_get_easing_sleep(&servo.internal.tilt) : 50);
            }
        }

        servo_reverse_check(&servo);

        tracker_check_atp_cmd(t);
        tracker_check_atp_ctr(t);

        now = time_millis_now();

        if (now < servo.internal.tilt.next_tick && now < servo.internal.pan.next_tick)
        {
            vTaskDelay(MILLIS_TO_TICKS(min(servo.internal.tilt.next_tick - now, servo.internal.pan.next_tick - now)));
        }

        hal_wd_feed();
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

float get_plane_lat()
{
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)) / 10000000.0f;
}

float get_plane_lon()
{
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)) / 10000000.0f;
}

float get_plane_alt()
{
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE)) / 10000000.0f;
}

float get_tracker_lat()
{
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE)) / 10000000.0f;
}

float get_tracker_lon()
{
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE)) / 10000000.0f;
}

float get_tracker_alt()
{
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE)) / 100.0f;
}

void tracker_pan_move(tracker_t *t, int v)
{
    if (t->servo->internal.pan.currtent_degree == 0 && v < 0)
    {
        t->servo->internal.pan.currtent_degree = 359;
    }
    else if (t->servo->internal.pan.currtent_degree >= 359 && v > 0)
    {
        t->servo->internal.pan.currtent_degree = 0;
    }
    else
    {
        t->servo->internal.pan.currtent_degree += v;
    }

    servo_pulsewidth_control(&t->servo->internal.pan, &t->servo->internal.ease_config);
}

void tracker_tilt_move(tracker_t *t, int v)
{
    if (t->servo->internal.tilt.currtent_degree == 0 && v < 0)
    {
        t->servo->internal.tilt.currtent_degree = 0;
    }
    else if (t->servo->internal.tilt.currtent_degree >= 90 && v > 0)
    {
        t->servo->internal.tilt.currtent_degree = 90;
    }
    else
    {
        t->servo->internal.tilt.currtent_degree += v;
    }

    servo_pulsewidth_control(&t->servo->internal.tilt, &t->servo->internal.ease_config);
}