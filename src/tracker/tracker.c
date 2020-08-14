#include <hal/log.h>
#include <hal/wd.h>

#include "util/macros.h"
#include "config/settings.h"
#include "tracker.h"
#include "protocols/atp.h"
#include "util/calc.h"
#include "sensors/imu_task.h"

static const char *TAG = "Tarcker";
static servo_t servo;
static atp_t atp;
static location_estimate_t estimate[MAX_ESTIMATE_COUNT];
static uint8_t estimate_index;

static int PROTOCOL_BAUDRATE[] = { PROTOCOL_BAUDRATE_1200, PROTOCOL_BAUDRATE_2400, PROTOCOL_BAUDRATE_4800, PROTOCOL_BAUDRATE_9600, PROTOCOL_BAUDRATE_19200, PROTOCOL_BAUDRATE_38400, PROTOCOL_BAUDRATE_57600,PROTOCOL_BAUDRATE_115200 };
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
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_DIRECTION))
    {
        t->servo->internal.pan.config.direction = setting_get_u8(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_TILT_DIRECTION))
    {
        t->servo->internal.tilt.config.direction = setting_get_u8(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH))
    {
        t->servo->internal.pan.config.max_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.pan.currtent_pulsewidth > t->servo->internal.pan.config.max_pulsewidth)
            t->servo->internal.pan.currtent_pulsewidth = t->servo->internal.pan.config.max_pulsewidth;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH))
    {
        t->servo->internal.pan.config.min_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.pan.currtent_pulsewidth < t->servo->internal.pan.config.min_pulsewidth)
            t->servo->internal.pan.currtent_pulsewidth = t->servo->internal.pan.config.min_pulsewidth;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH))
    {
        t->servo->internal.tilt.config.max_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.tilt.currtent_pulsewidth > t->servo->internal.tilt.config.max_pulsewidth)
            t->servo->internal.tilt.currtent_pulsewidth = t->servo->internal.tilt.config.max_pulsewidth;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH))
    {
        t->servo->internal.tilt.config.min_pulsewidth = setting_get_u16(setting);
        if (t->servo->internal.tilt.currtent_pulsewidth < t->servo->internal.tilt.config.min_pulsewidth)
            t->servo->internal.tilt.currtent_pulsewidth = t->servo->internal.tilt.config.min_pulsewidth;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_OUT_TYPE))
    {
        t->servo->internal.ease_config.ease_out = setting_get_u8(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MAX_STEPS))
    {
        t->servo->internal.ease_config.max_steps = setting_get_u16(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MIN_PULSEWIDTH))
    {
        t->servo->internal.ease_config.min_pulsewidth = setting_get_u16(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_STEP_MS))
    {
        t->servo->internal.ease_config.step_ms = setting_get_u16(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MAX_MS))
    {
        t->servo->internal.ease_config.max_ms = setting_get_u16(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_EASE_MIN_MS))
    {
        t->servo->internal.ease_config.min_ms = setting_get_u16(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_HOME_SET))
    {
        if (t->internal.flag && TRACKER_FLAG_PLANESETED)
        {
            time_micros_t now = time_micros_now();

            telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)), now);
            setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_LAT), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)));
            t->atp->tag_value_changed(t, TAG_TRACKER_LATITUDE);

            telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)), now);
            setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_LON), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)));
            t->atp->tag_value_changed(t, TAG_TRACKER_LONGITUDE);

            telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE)), now);
            setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_ALT), telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE)));
            t->atp->tag_value_changed(t, TAG_TRACKER_ALTITUDE);
        }

        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_HOME_RECOVER))
    {
        time_micros_t now = time_micros_now();

        telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE), settings_get_key_i32(SETTING_KEY_TRACKER_HOME_LAT), now);
        t->atp->tag_value_changed(t, TAG_TRACKER_LATITUDE);
        telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE), settings_get_key_i32(SETTING_KEY_TRACKER_HOME_LON), now);
        t->atp->tag_value_changed(t, TAG_TRACKER_LONGITUDE);
        telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE), settings_get_key_i32(SETTING_KEY_TRACKER_HOME_ALT), now);
        t->atp->tag_value_changed(t, TAG_TRACKER_ALTITUDE);

        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_HOME_CLEAR))
    {
        time_micros_t now = time_micros_now();
        int32_t zero = 0;

        telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE), zero, now);
        setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_LAT), zero);

        telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE), zero, now);
        setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_LON), zero);

        telemetry_set_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE), zero, now);
        setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_ALT), zero);

        t->internal.flag_changed(t, TRACKER_FLAG_HOMESETED, 0);

        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_IMU_CALIBRATION_ACC))
    {
        imu_task_do_accel_calibration_init();
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_IMU_CALIBRATION_GYRO))
    {
        imu_task_do_gyro_calibration();
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_IMU_CALIBRATION_MAG))
    {
        imu_task_do_mag_calibration();
        return;
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

static void tracker_reconfigure_input(tracker_t *t, uart_t *uart)
{
    LOG_I(TAG, "Reconfigure input");

    union {
        input_mavlink_config_t mavlink;
        input_ltm_config_t ltm;
        input_nmea_config_t nmea;
    } input_config;

    if (uart->input != NULL)
    {
        input_close(uart->input, uart->input_config);
        uart->input = NULL;
        uart->input_config = NULL;
    }

    memset(&uart->inputs, 0, sizeof(uart->inputs));

    switch (uart->protocol)
    {
    case PROTOCOL_ATP:
        break;
    case PROTOCOL_MSP:
        break;
    case PROTOCOL_MAVLINK:
        LOG_I(TAG, "Set [UART%d] to [MAVLINK] for input.", uart->com);
        input_mavlink_init(&uart->inputs.mavlink);
        uart->input = (input_t *)&uart->inputs.mavlink;
        input_config.mavlink.tx = uart->gpio_tx;
        input_config.mavlink.rx = uart->gpio_rx;
        input_config.mavlink.baudrate = uart->baudrate;
        uart->input_config = &input_config.mavlink;
        break;
    case PROTOCOL_LTM:
        LOG_I(TAG, "Set [UART%d] to [LTM] for input.", uart->com);
        input_ltm_init(&uart->inputs.ltm);
        uart->input = (input_t *)&uart->inputs.ltm;
        input_config.ltm.tx = uart->gpio_tx;
        input_config.ltm.rx = uart->gpio_rx;
        input_config.ltm.baudrate = uart->baudrate;
        uart->input_config = &input_config.ltm;
        break;
    case PROTOCOL_NMEA:
        LOG_I(TAG, "Set [UART%d] to [NMEA] for input.", uart->com);
        input_nmea_init(&uart->inputs.nmea);
        uart->input = (input_t *)&uart->inputs.nmea;
        input_config.nmea.tx = uart->gpio_tx;
        input_config.nmea.rx = uart->gpio_rx;
        input_config.nmea.baudrate = uart->baudrate;
        uart->input_config = &input_config.nmea;
        break;
    case PROTOCOL_PELCO_D:
        break;
    }

    if (uart->input != NULL)
    {
        uart->input->home_source = uart->com == settings_get_key_u8(SETTING_KEY_TRACKER_HOME_SOURCE);
        input_open(&t->atp, uart->input, uart->input_config);
    }
}

static void tracker_reconfigure_output(tracker_t *t, uart_t *uart)
{
    LOG_I(TAG, "Reconfigure output");

    union {
        output_pelco_d_config_t pelco_d;
    } output_config;

    if (uart->output != NULL)
    {
        output_close(uart->output, uart->output_config);
        uart->output = NULL;
        uart->output_config = NULL;
    }

    memset(&uart->outputs, 0, sizeof(uart->outputs));

    switch (uart->protocol)
    {
    case PROTOCOL_ATP:
        break;
    case PROTOCOL_MSP:
        break;
    case PROTOCOL_MAVLINK:
        break;
    case PROTOCOL_LTM:
        break;
    case PROTOCOL_NMEA:
        break;
    case PROTOCOL_PELCO_D:
        LOG_I(TAG, "Set [UART%d] to [PELCO_D] for output.", uart->com);
        output_pelco_d_init(&uart->outputs.pelco_d);
        uart->output = (output_t *)&uart->outputs.pelco_d;
        output_config.pelco_d.tx = uart->gpio_tx;
        output_config.pelco_d.rx = uart->gpio_rx;
        output_config.pelco_d.baudrate = uart->baudrate;
        uart->output_config = &output_config.pelco_d;
        break;
    }

    if (uart->output != NULL)
    {
        output_open(&t, uart->output, uart->output_config);
    }
}

void tracker_init(tracker_t *t)
{
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

    t->internal.show_coordinate = settings_get_key_bool(SETTING_KEY_TRACKER_SHOW_COORDINATE);
    t->internal.real_alt = settings_get_key_bool(SETTING_KEY_TRACKER_REAL_ALT);
    t->internal.estimate_location = settings_get_key_bool(SETTING_KEY_TRACKER_ESTIMATE_ENABLE);
    t->internal.advanced_position = settings_get_key_bool(SETTING_KEY_TRACKER_ADVANCED_POS_ENABLE);
    t->internal.eastimate_time = settings_get_key_u8(SETTING_KEY_TRACKER_ESTIMATE_SECOND);
    t->internal.advanced_time = settings_get_key_u16(SETTING_KEY_TRACKER_ADVANCED_POS_SECOND);
    t->internal.estimate = &estimate[0];
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

    t->uart1.com = 1;
    t->uart1.input_config = NULL;
    t->uart1.input = NULL;
    t->uart1.output_config = NULL;
    t->uart1.output = NULL;
    t->uart1.io_runing = false;
    t->uart1.invalidate_input = true;
    t->uart1.invalidate_output = true;
    t->uart1.gpio_tx = UART1_TX_DEFAULT_GPIO;
    t->uart1.gpio_rx = UART1_RX_DEFAULT_GPIO;
    t->uart1.baudrate = PROTOCOL_BAUDRATE[settings_get_key_u8(SETTING_KEY_PORT_UART1_BAUDRATE)];
    t->uart1.protocol = settings_get_key_u8(SETTING_KEY_PORT_UART1_PROTOCOL);
    t->uart1.io_type = settings_get_key_u8(SETTING_KEY_PORT_UART1_TYPE);

    t->uart2.com = 2;
    t->uart2.input_config = NULL;
    t->uart2.input = NULL;
    t->uart2.output_config = NULL;
    t->uart2.output = NULL;
    t->uart2.io_runing = false;
    t->uart2.invalidate_input = true;
    t->uart2.invalidate_output = true;
    t->uart2.gpio_tx = UART2_TX_DEFAULT_GPIO;
    t->uart2.gpio_rx = UART2_RX_DEFAULT_GPIO;
    t->uart2.baudrate = PROTOCOL_BAUDRATE[settings_get_key_u8(SETTING_KEY_PORT_UART2_BAUDRATE)];
    t->uart2.protocol = settings_get_key_u8(SETTING_KEY_PORT_UART2_PROTOCOL);
    t->uart2.io_type = settings_get_key_u8(SETTING_KEY_PORT_UART2_TYPE);

    memset(&estimate, 0, sizeof(location_estimate_t) * MAX_ESTIMATE_COUNT);
    estimate_index = 0;

    settings_add_listener(tracker_settings_handler, t);

    servo_init(&servo);
    atp_init(&atp);
}

void tracker_uart_update(tracker_t *t, uart_t *uart)
{
    if (UNLIKELY(uart->invalidate_input) && LIKELY(uart->io_type == PROTOCOL_IO_INPUT))
    {
        tracker_reconfigure_input(t, uart);
        uart->invalidate_input = false;
    }

        if (UNLIKELY(uart->invalidate_output) && LIKELY(uart->io_type == PROTOCOL_IO_OUTPUT))
    {
        tracker_reconfigure_output(t, uart);
        uart->invalidate_output = false;
    }

    if (LIKELY(uart->input != NULL))
    {
        time_micros_t now = time_micros_now();
        uart->input->vtable.update(uart->input, t->atp, now);
    }

    if (LIKELY(uart->output != NULL))
    {
        time_micros_t now = time_micros_now();
        uart->output->vtable.update(uart->output, t, now);
    }
}

void tracker_task(void *arg)
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
                LOG_D(TAG, "[pan] positon:%d -> to:%d | sleep:%dms | pwm:%d", servo.internal.pan.step_positon, servo.internal.pan.step_to, servo.internal.pan.step_sleep_ms, servo.internal.pan.last_pulsewidth);
            }
            else
            {
                if (t->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED) && t->internal.status == TRACKER_STATUS_TRACKING)
                {
                    float plane_lat = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)) /  10000000.0f;
                    float plane_lon = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)) /  10000000.0f;

                    //Estimate the vehicle's advance position
                    if (t->internal.advanced_position)
                    {
                        float dist = telemetry_get_i16(atp_get_telemetry_tag_val(TAG_PLANE_SPEED)) * (t->internal.advanced_time / 1250.0f);

                        float advanced_plane_lat = 0;
                        float advanced_plane_lon = 0;

                        distance_move_to(plane_lat, plane_lon, telemetry_get_u16(atp_get_telemetry_tag_val(TAG_PLANE_HEADING)), dist / 1000.0f, 
                            &advanced_plane_lat, &advanced_plane_lon);

                        LOG_D(TAG, "[Adv Pos] plane_lat:%f, plane_lon:%f, new_plane_lat:%f, new_plane_lon:%f", plane_lat, plane_lon, advanced_plane_lat, advanced_plane_lon);

                        plane_lat = advanced_plane_lat;
                        plane_lon = advanced_plane_lon;
                    }

                    //Estimate the position of the vehicle at the next time point
                    if (t->internal.estimate_location)
                    {
                        if (plane_lat == t->internal.estimate[estimate_index].latitude && plane_lon == t->internal.estimate[estimate_index].longitude)
                        {
                            time_millis_t move_time = now - t->internal.estimate[estimate_index].location_time;

                            if (move_time > (t->internal.eastimate_time * 1000)) move_time = t->internal.eastimate_time * 1000;

                            float dist = telemetry_get_i16(atp_get_telemetry_tag_val(TAG_PLANE_SPEED)) * (move_time / 1250.0f);

                            LOG_D(TAG, "[Est Loc] p_speed:%d, p_heading:%d, now:%d, location_time:%d, move_time:%d, move_dist:%f", 
                                telemetry_get_i16(atp_get_telemetry_tag_val(TAG_PLANE_SPEED)), 
                                telemetry_get_u16(atp_get_telemetry_tag_val(TAG_PLANE_HEADING)),
                                now,
                                t->internal.estimate[estimate_index].location_time,
                                move_time,
                                dist);

                            float estimate_plane_lat = 0;
                            float estimate_plane_lon = 0;

                            distance_move_to(plane_lat, plane_lon, telemetry_get_u16(atp_get_telemetry_tag_val(TAG_PLANE_HEADING)), dist / 1000.0f, 
                                &estimate_plane_lat, &estimate_plane_lon);

                            LOG_D(TAG, "[Est Loc] plane_lat:%f, plane_lon:%f, new_plane_lat:%f, new_plane_lon:%f", plane_lat, plane_lon, estimate_plane_lat, estimate_plane_lon);

                            plane_lat = estimate_plane_lat;
                            plane_lon = estimate_plane_lon;
                        }
                        else
                        {
                            LOG_D(TAG, "[Est Loc] estimate_index:%d", estimate_index);
                            estimate_index++;
                            if (estimate_index > 4) estimate_index = 0;
                            t->internal.estimate[estimate_index].latitude = plane_lat;
                            t->internal.estimate[estimate_index].longitude = plane_lon;
                            t->internal.estimate[estimate_index].location_time = now;
                            t->internal.estimate[estimate_index].speed = telemetry_get_i16(atp_get_telemetry_tag_val(TAG_PLANE_SPEED));
                            t->internal.estimate[estimate_index].direction = telemetry_get_u16(atp_get_telemetry_tag_val(TAG_PLANE_HEADING));
                        }
                    }
                    
                    float tracker_lat = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE)) / 10000000.0f;
                    float tracker_lon = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE)) / 10000000.0f;

                    distance = distance_between(tracker_lat, tracker_lon, plane_lat, plane_lon);

                    LOG_D(TAG, "[pan] t_lat:%f | t_lon:%f | p_lat:%f | p_lon:%f | dist:%d", tracker_lat, tracker_lon, plane_lat, plane_lon, distance);

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
                LOG_D(TAG, "[tilt] positon:%d -> to:%d | sleep:%dms | pwm:%d", servo.internal.tilt.step_positon, servo.internal.tilt.step_to, servo.internal.tilt.step_sleep_ms, servo.internal.tilt.last_pulsewidth );
            }
            else
            {
                if (t->internal.flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED) && t->internal.status == TRACKER_STATUS_TRACKING)
                {
                    int32_t tracker_alt =  t->internal.real_alt ? 0 : telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE));
                    int32_t plane_alt = telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE));

                    uint16_t tilt_deg = tilt_to(distance, tracker_alt, plane_alt);

                    LOG_D(TAG, "[tilt] t_alt:%d | p_alt:%d | dist:%d | tilt_deg:%d", tracker_alt, plane_alt, distance, tilt_deg);

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
    return telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_ALTITUDE)) / 100.0f;
}

float get_plane_speed()
{
    return (telemetry_get_i16(atp_get_telemetry_tag_val(TAG_PLANE_SPEED)) * 3600) / 1000.0f;
}

uint16_t get_plane_direction()
{
    return telemetry_get_u16(atp_get_telemetry_tag_val(TAG_PLANE_HEADING));
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

float get_tracker_roll()
{
    return telemetry_get_float(atp_get_telemetry_tag_val(TAG_TRACKER_ROLL));
}

float get_tracker_pitch()
{
    return telemetry_get_float(atp_get_telemetry_tag_val(TAG_TRACKER_PITCH));
}

float get_tracker_yaw()
{
    return telemetry_get_float(atp_get_telemetry_tag_val(TAG_TRACKER_YAW));
}

uint32_t get_tracker_imu_hz()
{
    return telemetry_get_u32(atp_get_telemetry_tag_val(TAG_TRACKER_IMU_HZ));
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