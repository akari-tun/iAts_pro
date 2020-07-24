#include <hal/log.h>
#include <string.h>
#include "atp.h"
#include "tracker/observer.h"
#include "config/settings.h"

static const char *TAG = "Protocol.Atp";
static telemetry_t plane_vals[TAG_PLANE_COUNT];
static telemetry_t tracker_vals[TAG_TRACKER_COUNT];
static telemetry_t param_vals[TAG_PARAM_COUNT];
// static telemetry_t iats_pro_param_vals[TAG_PARAM_IATS_PRO_COUNT];
static atp_cmd_t atp_cmd;
static atp_ctr_t atp_ctr;
static atp_t *atp;

typedef union
{
	float fdata;
	unsigned long ldata;
}FloatLongType;

static const atp_tag_info_t atp_tag_infos[] = {
    { TAG_PLANE_LONGITUDE,                         TELEMETRY_TYPE_INT32,   "Lon",            telemetry_format_coordinate },                 
    { TAG_PLANE_LATITUDE,                          TELEMETRY_TYPE_INT32,   "Lat",            telemetry_format_coordinate },
    { TAG_PLANE_ALTITUDE,                          TELEMETRY_TYPE_INT32,   "Alt",            telemetry_format_altitude },
    { TAG_PLANE_SPEED,                             TELEMETRY_TYPE_INT16,   "Spd",            telemetry_format_horizontal_speed },
    { TAG_PLANE_DISTANCE,                          TELEMETRY_TYPE_UINT32,  "Dist",           telemetry_format_metre },
    { TAG_PLANE_STAR,                              TELEMETRY_TYPE_INT16,   "GPS Sta.",       telemetry_format_u8 },
    { TAG_PLANE_FIX,                               TELEMETRY_TYPE_UINT8,   "GPS Fix.",       telemetry_format_gps_fix },
    { TAG_PLANE_PITCH,                             TELEMETRY_TYPE_INT16,   "Pitch",          telemetry_format_deg },
    { TAG_PLANE_ROLL,                              TELEMETRY_TYPE_INT16,   "Roll",           telemetry_format_deg },
    { TAG_PLANE_HEADING,                           TELEMETRY_TYPE_UINT16,  "Heading",        telemetry_format_deg },
    { TAG_TRACKER_LONGITUDE,                       TELEMETRY_TYPE_INT32,   "Lon",            telemetry_format_coordinate },
    { TAG_TRACKER_LATITUDE,                        TELEMETRY_TYPE_INT32,   "Lat",            telemetry_format_coordinate },
    { TAG_TRACKER_ALTITUDE,                        TELEMETRY_TYPE_INT32,   "Alt",            telemetry_format_altitude },
    { TAG_TRACKER_HEADING,                         TELEMETRY_TYPE_UINT16,  "Heading",        telemetry_format_deg },
    { TAG_TRACKER_PITCH,                           TELEMETRY_TYPE_FLOAT,   "Pitch",          telemetry_format_ahrs },
    { TAG_TRACKER_VOLTAGE,                         TELEMETRY_TYPE_UINT16,  "Batt. V.",       telemetry_format_voltage },
    { TAG_TRACKER_MODE,                            TELEMETRY_TYPE_UINT8,   "Mode",           telemetry_format_tracker_mode },
    { TAG_TRACKER_DECLINATION,                     TELEMETRY_TYPE_INT8,    "Dec.",           telemetry_format_deg},
    { TAG_TRACKER_T_IP,                            TELEMETRY_TYPE_UINT32,  "T IP.",          telemetry_format_ip },
    { TAG_TRACKER_T_PORT,                          TELEMETRY_TYPE_UINT16,  "T Port.",        telemetry_format_u16 },
    { TAG_TRACKER_S_IP,                            TELEMETRY_TYPE_UINT32,  "S IP.",          telemetry_format_ip },
    { TAG_TRACKER_S_PORT,                          TELEMETRY_TYPE_UINT16,  "S Port.",        telemetry_format_u16 },
    { TAG_TRACKER_FLAG,                            TELEMETRY_TYPE_UINT8,   "Flag",           telemetry_format_u8 },
    { TAG_TRACKER_SEVRO_POWER,                     TELEMETRY_TYPE_UINT8,   "S. Power",       telemetry_format_u8 },
    { TAG_TRACKER_ROLL,                            TELEMETRY_TYPE_FLOAT,   "ROLL",           telemetry_format_ahrs },
    { TAG_TRACKER_YAW,                             TELEMETRY_TYPE_FLOAT,   "YAW",            telemetry_format_ahrs },
    { TAG_PARAM_PID_P,                             TELEMETRY_TYPE_UINT16,  "P",              telemetry_format_u16 },
    { TAG_PARAM_PID_I,                             TELEMETRY_TYPE_UINT16,  "I",              telemetry_format_u16 },
    { TAG_PARAM_PID_D,                             TELEMETRY_TYPE_UINT16,  "D",              telemetry_format_u16 },
    { TAG_PARAM_TITL_0,                            TELEMETRY_TYPE_UINT16,  "Titl 0",         telemetry_format_u16 },
    { TAG_PARAM_TITL_90,                           TELEMETRY_TYPE_UINT16,  "Titl 90",        telemetry_format_u16 },
    { TAG_PARAM_PAN_0,                             TELEMETRY_TYPE_UINT16,  "Pan 0",          telemetry_format_u16 },
    { TAG_PARAM_OFFSET,                            TELEMETRY_TYPE_INT16,   "Offset",         telemetry_format_deg },
    { TAG_PARAM_TRACKING_DISTANCE,                 TELEMETRY_TYPE_UINT8,   "Start m.",       telemetry_format_metre },
    { TAG_PARAM_MAX_PID_ERROR,                     TELEMETRY_TYPE_UINT16,  "PID err.",       telemetry_format_u16 },
    { TAG_PARAM_MIN_PAN_SPEED,                     TELEMETRY_TYPE_UINT8,   "Pan spd.",       telemetry_format_u8 },
    { TAG_PARAM_DECLINATION,                       TELEMETRY_TYPE_INT8,    "Decl.",          telemetry_format_deg },
    // { TAG_PARAM_SHOW_COORDINATE,                   TELEMETRY_TYPE_UINT8,   "Show C.",        telemetry_format_u8 },     //是否在主界面显示坐标 L:1
    // { TAG_PARAM_MONITOR_BATTERY_ENABLE,            TELEMETRY_TYPE_UINT8,   "Bat.Enable",     telemetry_format_u8 },     //是否监控电池电压 L:1
    // { TAG_PARAM_MONITOR_BATTERY_VOLTAGE_SCALE,     TELEMETRY_TYPE_UINT16,  "Bat.Scale",      telemetry_format_u16 },    //电池电压分压系数 L:2
    // { TAG_PARAM_MONITOR_BATTERY_MAX_VOLTAGE,       TELEMETRY_TYPE_UINT16,  "Bat.V.Max",      telemetry_format_u16 },    //电池电压最大值 L:2
    // { TAG_PARAM_MONITOR_BATTERY_MIN_VOLTAGE,       TELEMETRY_TYPE_UINT16,  "Bat.V.Min",      telemetry_format_u16 },    //电池电压最小值 L:2
    // { TAG_PARAM_MONITOR_BATTERY_CENTER_VOLTAGE,    TELEMETRY_TYPE_UINT16,  "Bat.V.C",        telemetry_format_u16 },    //电池电压中间值 L:2
    // { TAG_PARAM_MONITOR_POWER_ENABLE,              TELEMETRY_TYPE_UINT8,   "Pwr.Enable",     telemetry_format_u8 },     //监控舵机电源 L:1
    // { TAG_PARAM_WIFI_SSID,                         TELEMETRY_TYPE_STRING,  "Wifi SSID",      telemetry_format_str },    //WIFI SSID L:32
    // { TAG_PARAM_WIFI_PWD,                          TELEMETRY_TYPE_STRING,  "Wifi PWD",       telemetry_format_str },    //WIFI PWD L:32
    // { TAG_PARAM_SERVO_COURSE,                      TELEMETRY_TYPE_UINT16,  "S.Course",       telemetry_format_u16 },    //正北位置舵机指向 L:2
    // { TAG_PARAM_SERVO_PAN_MIN_PLUSEWIDTH,          TELEMETRY_TYPE_UINT16,  "S.P.PWM.MIN",    telemetry_format_u16 },    //水平舵机最小PWM L:2
    // { TAG_PARAM_SERVO_PAN_MAX_PLUSEWIDTH,          TELEMETRY_TYPE_UINT16,  "S.P.PWM.MAX",    telemetry_format_u16 },    //水平舵机最大PWM L:2
    // { TAG_PARAM_SERVO_PAN_MAX_DEGREE,              TELEMETRY_TYPE_UINT16,  "S.P.DEG.MAX",    telemetry_format_u16 },    //水平舵机最大角度 L:2
    // { TAG_PARAM_SERVO_PAN_MIN_DEGREE,              TELEMETRY_TYPE_UINT16,  "S.P.DEG.MIN",    telemetry_format_u16 },    //水平舵机最小角度 L:2
    // { TAG_PARAM_SERVO_PAN_DIRECTION,  TELEMETRY_TYPE_UINT8,   "S.P.Z.PWM",      telemetry_format_u8 },     //水平舵机零度PWM值 L:1
    // { TAG_PARAM_SERVO_TILT_MIN_PLUSEWIDTH,         TELEMETRY_TYPE_UINT16,  "S.T.PWM.MIN",    telemetry_format_u16 },    //俯仰舵机最小PWM L:2
    // { TAG_PARAM_SERVO_TILT_MAX_PLUSEWIDTH,         TELEMETRY_TYPE_UINT16,  "S.T.PWM.MAX",    telemetry_format_u16 },    //俯仰舵机最大PWM L:2
    // { TAG_PARAM_SERVO_TILT_MAX_DEGREE,             TELEMETRY_TYPE_UINT16,  "S.T.DEG.MAX",    telemetry_format_u16 },    //俯仰舵机最大角度 L:2
    // { TAG_PARAM_SERVO_TILT_MIN_DEGREE,             TELEMETRY_TYPE_UINT16,  "S.T.DEG.MIN",    telemetry_format_u16 },    //俯仰舵机最小角度 L:2
    // { TAG_PARAM_SERVO_TILT_DIRECTION, TELEMETRY_TYPE_UINT8,   "S.T.Z.PWM",      telemetry_format_u8 },     //俯仰舵机零度PWM值 L:1
    // { TAG_PARAM_SERVO_EASE_OUT_TYPE,               TELEMETRY_TYPE_UINT8,   "S.E.T",          telemetry_format_u8 },     //缓冲类型 L:1
    // { TAG_PARAM_SERVO_EASE_MAX_STEPS,              TELEMETRY_TYPE_UINT16,  "S.E.T.MAX",      telemetry_format_u16 },    //缓冲最大步数 L:2
    // { TAG_PARAM_SERVO_EASE_MIN_PULSEWIDTH,         TELEMETRY_TYPE_UINT16,  "S.E.PWM.MIN",    telemetry_format_u16 },    //缓冲最小PWM L:2
    // { TAG_PARAM_SERVO_EASE_STEP_MS,                TELEMETRY_TYPE_UINT16,  "S.E.T.S",        telemetry_format_u16 },    //缓冲每步间隔（毫秒） L:2
    // { TAG_PARAM_SERVO_EASE_MAX_MS,                 TELEMETRY_TYPE_UINT16,  "S.E.S.MAX",      telemetry_format_u16 },    //缓冲最大时间（毫秒） L:2
    // { TAG_PARAM_SERVO_EASE_MIN_MS,                 TELEMETRY_TYPE_UINT16,  "S.E.S.MIN",      telemetry_format_u16 },    //缓冲最小时间（毫秒） L:2
    // { TAG_PARAM_SCREEN_BRIGHTNESS,                 TELEMETRY_TYPE_UINT8,   "SC.BRI.",        telemetry_format_u8 },     //屏幕亮度 L:1
    // { TAG_PARAM_SCREEN_AUTO_OFF,                   TELEMETRY_TYPE_UINT8,   "SC.A.OFF",       telemetry_format_u8 },     //自动关屏 L:1
    // { TAG_PARAM_BEEPER_ENABLE,                     TELEMETRY_TYPE_UINT8,   "B.Enable",       telemetry_format_u8 },     //Beeper L:1
};

static uint8_t tagread_u8(atp_frame_t *frame)
{
    return frame->buffer[frame->buffer_index++];
}

static uint16_t tagread_u16(atp_frame_t *frame)
{
    uint16_t t = tagread_u8(frame);
    t |= (uint16_t)tagread_u8(frame) << 8;
    return t;
}

static uint32_t tagread_u32(atp_frame_t *frame)
{
    uint32_t t = tagread_u16(frame);
    t |= (uint32_t)tagread_u16(frame) << 16;
    return t;
}

static void tag_write_telemetry(atp_frame_t *frame, telemetry_t *val)
{
    switch (val->type)
    {
        case TELEMETRY_TYPE_UINT8:
            frame->buffer[frame->buffer_index++] = 1;
            frame->buffer[frame->buffer_index++] = telemetry_get_u8(val);
            break;
        case TELEMETRY_TYPE_INT8:
            frame->buffer[frame->buffer_index++] = 1;
            frame->buffer[frame->buffer_index++] = telemetry_get_i8(val);
            break;
        case TELEMETRY_TYPE_UINT16:
            frame->buffer[frame->buffer_index++] = 2;
            uint16_t val_u16 = telemetry_get_u16(val);
            frame->buffer[frame->buffer_index++] = (val_u16 >> 8 * 0) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_u16 >> 8 * 1) & 0xFF;
            break;
        case TELEMETRY_TYPE_INT16:
            frame->buffer[frame->buffer_index++] = 2;
            uint16_t val_i16 = telemetry_get_i16(val);
            frame->buffer[frame->buffer_index++] = (val_i16 >> 8 * 0) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_i16 >> 8 * 1) & 0xFF;
            break;
        case TELEMETRY_TYPE_UINT32:
            frame->buffer[frame->buffer_index++] = 4;
            uint32_t val_u32 = telemetry_get_u32(val);
            frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 0) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 1) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 2) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 3) & 0xFF;
            break;
        case TELEMETRY_TYPE_INT32:
            frame->buffer[frame->buffer_index++] = 4;
            uint32_t val_i32 = telemetry_get_i32(val);
            frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 0) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 1) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 2) & 0xFF;
            frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 3) & 0xFF;
            break;
        case TELEMETRY_TYPE_FLOAT:
            frame->buffer[frame->buffer_index++] = 4;
            FloatLongType fl;
	        fl.fdata = telemetry_get_float(val);
            frame->buffer[frame->buffer_index++] = (unsigned char)(fl.ldata);
            frame->buffer[frame->buffer_index++] = (unsigned char)(fl.ldata >> 8);
            frame->buffer[frame->buffer_index++] = (unsigned char)(fl.ldata >> 16);
            frame->buffer[frame->buffer_index++] = (unsigned char)(fl.ldata >> 24);
            break;
        case TELEMETRY_TYPE_STRING:

            break;
    }
}

static void tag_write_setting(atp_frame_t *frame, const char *key)
{
    const setting_t *setting = settings_get_key(key);

    switch (setting->type)
    {
    case SETTING_TYPE_U8:
        frame->buffer[frame->buffer_index++] = 1;
        frame->buffer[frame->buffer_index++] = setting_get_u8(setting);
        break;
    case SETTING_TYPE_I8:
        frame->buffer[frame->buffer_index++] = 1;
        frame->buffer[frame->buffer_index++] = setting_get_i8(setting);
        break;
    case SETTING_TYPE_U16:
        frame->buffer[frame->buffer_index++] = 2;
        uint16_t val_u16 = setting_get_u16(setting);
        frame->buffer[frame->buffer_index++] = (val_u16 >> 8 * 0) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_u16 >> 8 * 1) & 0xFF;
        break;
    case SETTING_TYPE_I16:
        frame->buffer[frame->buffer_index++] = 2;
        uint16_t val_i16 = setting_get_i16(setting);
        frame->buffer[frame->buffer_index++] = (val_i16 >> 8 * 0) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_i16 >> 8 * 1) & 0xFF;
        break;
    case SETTING_TYPE_U32:
        frame->buffer[frame->buffer_index++] = 4;
        uint32_t val_u32 = setting_get_u32(setting);
        frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 0) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 1) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 2) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_u32 >> 8 * 3) & 0xFF;
        break;
    case SETTING_TYPE_I32:
        frame->buffer[frame->buffer_index++] = 4;
        uint32_t val_i32 = setting_get_i32(setting);
        frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 0) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 1) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 2) & 0xFF;
        frame->buffer[frame->buffer_index++] = (val_i32 >> 8 * 3) & 0xFF;
        break;
    case SETTING_TYPE_STRING:
        frame->buffer[frame->buffer_index] = 0;
        size_t len = strlen(setting_get_string(setting));
        frame->buffer[frame->buffer_index++] = len;
        memcpy(&frame->buffer[frame->buffer_index], setting_get_string(setting), len);
        frame->buffer_index += len;
        break;
    default:
        break;
    }
}

static void setting_write(atp_frame_t *frame, const char *key)
{
    const setting_t *setting = settings_get_key(key);

    switch (setting->type)
    {
    case SETTING_TYPE_U8:
        frame->buffer_index++;
        setting_set_u8(setting, tagread_u8(frame));
        break;
    case SETTING_TYPE_I8:
        frame->buffer_index++;
        setting_set_u8(setting, (int8_t)tagread_u8(frame));
        break;
    case SETTING_TYPE_U16:
        frame->buffer_index++;
        setting_set_u16(setting, tagread_u16(frame));
        break;
    case SETTING_TYPE_I16:
        frame->buffer_index++;
        setting_set_u16(setting, (int16_t)tagread_u16(frame));
        break;
    case SETTING_TYPE_U32:
        frame->buffer_index++;
        setting_set_u32(setting, tagread_u32(frame));
        break;
    case SETTING_TYPE_I32:
        frame->buffer_index++;
        setting_set_u32(setting, (int32_t)tagread_u32(frame));
        break;
    case SETTING_TYPE_STRING:
        frame->buffer_index++;
        size_t len = frame->buffer[frame->buffer_index - 1];
        char *str = (char *)malloc(len);
        memcpy(&frame->buffer + frame->buffer_index, str, len);
        setting_set_string(setting, str);
        free(str);
        frame->buffer_index += len;
        break;
    default:
        break;
    }
}

static void atp_add_control(uint8_t v, void *data, uint8_t len)
{
    for (int i = 0; i < 5; i++)
    {
        if (atp->atp_ctr->ctrs[i] == 0)
        {
            atp->atp_ctr->ctrs[i] = v;
            memcpy((&atp->atp_ctr->data) + atp->atp_ctr->data_index, data, len);
            break;
        }
    }
}

static void atp_cmd_ack(atp_frame_t *frame)
{
    while (frame->buffer_index < frame->atp_tag_len + 5)
    {
        switch (frame->buffer[frame->buffer_index++])
        {
        case TAG_BASE_ACK:
            frame->buffer_index++;
            uint8_t tag = tagread_u8(frame);
            UNUSED(tag);
            atp->tag_value_changed(atp->tracker, TAG_BASE_ACK);
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_cmd_heartbeat(atp_frame_t *frame)
{
    while (frame->buffer_index < frame->atp_tag_len + 5)
    {
        switch (frame->buffer[frame->buffer_index++])
        {
        case TAG_BASE_ACK:
            frame->buffer_index++;
            // tracker->internal.status = tagread_u8(frame);
            // if (tracker->internal.status >= 3)
                // tracker->last_heartbeat = time_millis_now();
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_cmd_airplane(atp_frame_t *frame)
{
    time_micros_t now = time_micros_now();

    while (frame->buffer_index < frame->atp_tag_len + 5)
    {
        switch (frame->buffer[frame->buffer_index++])
        {
        case TAG_PLANE_LONGITUDE:   //plane's longitude L:4
            frame->buffer_index++;
            ATP_SET_I32(TAG_PLANE_LONGITUDE, (int32_t)tagread_u32(frame), now);
            atp->tag_value_changed(atp->tracker, TAG_PLANE_LONGITUDE);
            // printf("TAG_PLANE_LONGITUDE:%d\n", telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LONGITUDE)));
            break;
        case TAG_PLANE_LATITUDE:    //plane's latitude L:4
            frame->buffer_index++;
            ATP_SET_I32(TAG_PLANE_LATITUDE, (int32_t)tagread_u32(frame), now);
            atp->tag_value_changed(atp->tracker, TAG_PLANE_LATITUDE);
            // printf("TAG_PLANE_LATITUDE:%d\n", telemetry_get_i32(atp_get_telemetry_tag_val(TAG_PLANE_LATITUDE)));
            break;
        case TAG_PLANE_ALTITUDE:    //plane's altitude L:4
            frame->buffer_index++;
            ATP_SET_I32(TAG_PLANE_ALTITUDE, (int32_t)tagread_u32(frame), now);
            break;
        case TAG_PLANE_SPEED:       //plane's speed L:2
            frame->buffer_index++;
            ATP_SET_I16(TAG_PLANE_SPEED, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_DISTANCE:    //plane's distance L:4
            frame->buffer_index++;
            ATP_SET_U32(TAG_PLANE_DISTANCE, tagread_u32(frame), now);
            break;
        case TAG_PLANE_STAR:        //plane's star numbers L:1
            frame->buffer_index++;
            ATP_SET_U8(TAG_PLANE_STAR, tagread_u8(frame), now);
            break;
        case TAG_PLANE_FIX:         //plane's fix type L:1
            frame->buffer_index++;
            ATP_SET_U8(TAG_PLANE_FIX, tagread_u8(frame), now);
            break;
        case TAG_PLANE_PITCH:       //plane's pitch L:2
            frame->buffer_index++;
            ATP_SET_I16(TAG_PLANE_PITCH, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_ROLL:        //plane's roll L:2
            frame->buffer_index++;
            ATP_SET_I16(TAG_PLANE_ROLL, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_HEADING:     //plane's heading L:2
            frame->buffer_index++;
            ATP_SET_U16(TAG_PLANE_HEADING, tagread_u16(frame), now);
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_cmd_sethome(atp_frame_t *frame)
{
    time_micros_t now = time_micros_now();

    while (frame->buffer_index < frame->atp_tag_len + 5)
    {
        // printf("buffer_index:%d\n", frame->buffer_index);
        switch (frame->buffer[frame->buffer_index++])
        {
        case TAG_TRACKER_LONGITUDE: //tarcker's longitude L:4
            frame->buffer_index++;
            int32_t lon = (int32_t)tagread_u32(frame);
            ATP_SET_I32(TAG_TRACKER_LONGITUDE, lon, now);
            atp->tag_value_changed(atp->tracker, TAG_TRACKER_LONGITUDE);
            setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_LON), lon);
            // printf("TAG_TRACKER_LONGITUDE:%d\n", telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LONGITUDE)));
            break;
        case TAG_TRACKER_LATITUDE: //tarcker's latitude L:4
            frame->buffer_index++;
            int32_t lat = (int32_t)tagread_u32(frame);
            ATP_SET_I32(TAG_TRACKER_LATITUDE, lat, now);
            atp->tag_value_changed(atp->tracker, TAG_TRACKER_LATITUDE);
            setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_LAT), lat);
            // printf("TAG_TRACKER_LATITUDE:%d\n", telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_LATITUDE)));
            break;
        case TAG_TRACKER_ALTITUDE: //tarcker's altitude L:4
            frame->buffer_index++;
            int32_t alt = (int32_t)tagread_u32(frame);
            ATP_SET_I32(TAG_TRACKER_ALTITUDE, alt, now);
            atp->tag_value_changed(atp->tracker, TAG_TRACKER_ALTITUDE);
            setting_set_i32(settings_get_key(SETTING_KEY_TRACKER_HOME_ALT), alt);
            // printf("TAG_TRACKER_ALTITUDE:%d\n", telemetry_get_i32(atp_get_telemetry_tag_val(TAG_TRACKER_ALTITUDE)));
            break;
        case TAG_TRACKER_MODE: //tarcker's mode L:1
            frame->buffer_index++;
            ATP_SET_U8(TAG_TRACKER_MODE, tagread_u8(frame), now);
            break;
        case TAG_TRACKER_DECLINATION: //tarcker's declination L:1
            frame->buffer_index++;
            ATP_SET_I8(TAG_TRACKER_DECLINATION, (int8_t)tagread_u8(frame), now);
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_cmd_setparam(atp_frame_t *frame)
{
    while (frame->buffer_index < frame->atp_tag_len + 5)
    {
        switch (frame->buffer[frame->buffer_index++])
        {
        // case TAG_PARAM_PID_P: //PID_P L:2
        //     frame->buffer_index++;
        //     // PTParam->pid_p = tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->pid_p, PARAM_EPPROM_POS_PID_P);
        //     break;
        // case TAG_PARAM_PID_I: //PID_I L:2
        //     frame->buffer_index++;
        //     // PTParam->pid_i = tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->pid_i, PARAM_EPPROM_POS_PID_I);
        //     break;
        // case TAG_PARAM_PID_D: //PID_D L:2
        //     frame->buffer_index++;
        //     // PTParam->pid_d = tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->pid_d, PARAM_EPPROM_POS_PID_D);
        //     break;
        // case TAG_PARAM_TITL_0: //俯仰零度 L:2
        //     frame->buffer_index++;
        //     // PTParam->tilt_0 = tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->tilt_0, PARAM_EPPROM_POS_TILT_0);
        //     break;
        // case TAG_PARAM_TITL_90: //俯仰90度 L:2
        //     frame->buffer_index++;
        //     // PTParam->tilt_90 = tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->tilt_90, PARAM_EPPROM_POS_TILT_90);
        //     break;
        // case TAG_PARAM_PAN_0: //水平中立点 L:2
        //     frame->buffer_index++;
        //     // PTParam->pan_center = tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->pan_center, PARAM_EPPROM_POS_PAN_CENTER);
        //     break;
        // case TAG_PARAM_OFFSET: //罗盘偏移量 L:2
        //     frame->buffer_index++;
        //     // PTParam->compass_offset = (int8_t)tagread_u16();
        //     // StoreToEEPROM_u16(PTParam->compass_offset, PARAM_EPPROM_POS_COMPASS_OFFSET);
        //     break;
        // case TAG_PARAM_TRACKING_DISTANCE: //开始跟踪距离 L:1
        //     frame->buffer_index++;
        //     // PTParam->start_tracking_distance = tagread_u8();
        //     // StoreToEEPROM_u8(PTParam->start_tracking_distance, PARAM_EPPROM_POS_START_TRACKING_DISTANCE);
        //     break;
        // case TAG_PARAM_MAX_PID_ERROR: //最大角度偏移 L:1
        //     frame->buffer_index++;
        //     // PTParam->pid_max_error = tagread_u8();
        //     // StoreToEEPROM_u8(PTParam->pid_max_error, PARAM_EPPROM_POS_PID_MAX_ERROR);
        //     break;
        // case TAG_PARAM_MIN_PAN_SPEED: //最小水平舵机速度 L:1
        //     frame->buffer_index++;
        //     // PTParam->pan_min_speed = tagread_u8();
        //     // StoreToEEPROM_u8(PTParam->pan_min_speed, PARAM_EPPROM_POS_PAN_MIN_SPEED);
        //     break;
        // case TAG_PARAM_DECLINATION: //磁偏角 L:1
        //     frame->buffer_index++;
        //     // PTParam->compass_declination = (int8_t)tagread_u8();
        //     // StoreToEEPROM_u8((uint8_t)PTParam->compass_declination, PARAM_EPPROM_POS_COMPASS_DECLINATION);
        //     break;
        case TAG_PARAM_SHOW_COORDINATE:                  //是否在主界面显示坐标 L:1
            setting_write(frame, SETTING_KEY_TRACKER_SHOW_COORDINATE);
            break;
        case TAG_PARAM_MONITOR_BATTERY_ENABLE:           //是否监控电池电压 L:1
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_ENABLE);
            break;
        case TAG_PARAM_MONITOR_BATTERY_VOLTAGE_SCALE:    //电池电压分压系数 L:2
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_VOLTAGE_SCALE);
            break;
        case TAG_PARAM_MONITOR_BATTERY_MAX_VOLTAGE:      //电池电压最大值 L:2
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_MAX_VOLTAGE);
            break;
        case TAG_PARAM_MONITOR_BATTERY_MIN_VOLTAGE:      //电池电压最小值 L:2
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_MIN_VOLTAGE);
            break;
        case TAG_PARAM_MONITOR_BATTERY_CENTER_VOLTAGE:   //电池电压中间值 L:2
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_CENTER_VOLTAGE);
            break;
        case TAG_PARAM_MONITOR_POWER_ENABLE:             //监控舵机电源 L:1
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_POWER_ENABLE);
            break;
        case TAG_PARAM_MONITOR_POWER_ON:                 //舵机电源打开 L:1
            setting_write(frame, SETTING_KEY_TRACKER_MONITOR_POWER_TURN);
            break;
        case TAG_PARAM_WIFI_SSID:                        //WIFI SSID L:32
        case TAG_PARAM_WIFI_PWD:                         //WIFI PWD L:32
            frame->buffer_index += frame->buffer[frame->buffer_index] + 1;
            break;
        case TAG_PARAM_SERVO_COURSE:                     //正北位置舵机指向 L:2
            setting_write(frame, SETTING_KEY_SERVO_COURSE);
            break;
        case TAG_PARAM_SERVO_PAN_MIN_PLUSEWIDTH:         //水平舵机最小PWM L:2
            setting_write(frame, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH);
            break;
        case TAG_PARAM_SERVO_PAN_MAX_PLUSEWIDTH:         //水平舵机最大PWM L:2
            setting_write(frame, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH);
            break;
        case TAG_PARAM_SERVO_PAN_MAX_DEGREE:             //水平舵机最大角度 L:2
            setting_write(frame, SETTING_KEY_SERVO_PAN_MAX_DEGREE);
            break;
        case TAG_PARAM_SERVO_PAN_MIN_DEGREE:             //水平舵机最小角度 L:2
            setting_write(frame, SETTING_KEY_SERVO_PAN_MIN_DEGREE);
            break;
        case TAG_PARAM_SERVO_PAN_DIRECTION:              //水平舵机方向 L:1
            setting_write(frame, SETTING_KEY_SERVO_PAN_DIRECTION);
            break;
        case TAG_PARAM_SERVO_TILT_MIN_PLUSEWIDTH:        //俯仰舵机最小PWM L:2
            setting_write(frame, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH);
            break;
        case TAG_PARAM_SERVO_TILT_MAX_PLUSEWIDTH:        //俯仰舵机最大PWM L:2
            setting_write(frame, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH);
            break;
        case TAG_PARAM_SERVO_TILT_MAX_DEGREE:            //俯仰舵机最大角度 L:2
            setting_write(frame, SETTING_KEY_SERVO_TILT_MAX_DEGREE);
            break;
        case TAG_PARAM_SERVO_TILT_MIN_DEGREE:            //俯仰舵机最小角度 L:2
            setting_write(frame, SETTING_KEY_SERVO_TILT_MIN_DEGREE);
            break;
        case TAG_PARAM_SERVO_TILT_DIRECTION:             //俯仰舵机方向 L:1
            setting_write(frame, SETTING_KEY_SERVO_TILT_DIRECTION);
            break;
        case TAG_PARAM_SERVO_EASE_OUT_TYPE:              //缓冲类型 L:1
            setting_write(frame, SETTING_KEY_SERVO_EASE_OUT_TYPE);
            break;
        case TAG_PARAM_SERVO_EASE_MAX_STEPS:             //缓冲最大步数 L:2
            setting_write(frame, SETTING_KEY_SERVO_EASE_MAX_STEPS);
            break;
        case TAG_PARAM_SERVO_EASE_MIN_PULSEWIDTH:        //缓冲最小PWM L:2
            setting_write(frame, SETTING_KEY_SERVO_EASE_MIN_PULSEWIDTH);
            break;
        case TAG_PARAM_SERVO_EASE_STEP_MS:               //缓冲每步间隔（毫秒） L:2
            setting_write(frame, SETTING_KEY_SERVO_EASE_STEP_MS);
            break;
        case TAG_PARAM_SERVO_EASE_MAX_MS:                //缓冲最大时间（毫秒） L:2
            setting_write(frame, SETTING_KEY_SERVO_EASE_MAX_MS);
            break;
        case TAG_PARAM_SERVO_EASE_MIN_MS:                //缓冲最小时间（毫秒） L:2
            setting_write(frame, SETTING_KEY_SERVO_EASE_MIN_MS);
            break;
        case TAG_PARAM_SCREEN_BRIGHTNESS:                //屏幕亮度 L:1
            setting_write(frame, SETTING_KEY_SCREEN_BRIGHTNESS);
            break;
        case TAG_PARAM_SCREEN_AUTO_OFF:                  //自动关屏 L:1
            setting_write(frame, SETTING_KEY_SCREEN_AUTO_OFF);
            break;
        case TAG_PARAM_BEEPER_ENABLE:                    //Beeper L:1
            setting_write(frame, SETTING_KEY_BEEPER_ENABLE);
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_cmd_control(atp_frame_t *frame)
{
    while (frame->buffer_index < frame->atp_tag_len + 5)
    {
        switch (frame->buffer[frame->buffer_index++])
        {
        case TAG_CTR_MODE: //设置模式 L:1
            frame->buffer_index++;
            // PTracker->mode = tagread_u8();
            // setReceiveCmd(TAG_CTR_MODE);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_MODE;
            break;
        case TAG_CTR_AUTO_POINT_TO_NORTH: //设置自动指北 L:1
            frame->buffer_index++;
            // AUTO_POINT_TO_NORTH = tagread_u8();
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_AUTO_POINT_TO_NORTH;
            break;
        case TAG_CTR_CALIBRATE: //校准 L:1
            frame->buffer_index++;
            // uint8_t cal;
            // cal = tagread_u8();
            // setReceiveCmd(TAG_CTR_CALIBRATE);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_CALIBRATE;
            break;
        case TAG_CTR_HEADING: //指向 L:2
            frame->buffer_index++;
            // PTracker->course = tagread_u16();
            // setReceiveCmd(TAG_CTR_HEADING);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_HEADING;
            break;
        case TAG_CTR_TILT: //俯仰 L:1
            frame->buffer_index++;
            // PTracker->pitching = tagread_u8();
            // setReceiveCmd(TAG_CTR_TILT);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_TILT;
            break;
        case TAG_CTR_REBOOT:
            frame->buffer_index++;
            uint8_t reboot_v = tagread_u8(frame);
            atp_add_control(TAG_CTR_REBOOT, &reboot_v, sizeof(reboot_v));
            LOG_I(TAG, "Set [REBOOT] command to stack -> %d", reboot_v);
            break;
        case TAG_CTR_SMART_CONFIG:
            frame->buffer_index++;
            uint8_t sc_v = tagread_u8(frame);
            atp_add_control(TAG_CTR_SMART_CONFIG, &sc_v, sizeof(sc_v));
            LOG_I(TAG, "Set [SMART_CONFIG] command to stack -> %d", sc_v);
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_tag_analysis(atp_frame_t *frame)
{
    switch (frame->atp_cmd)
    {
    case CMD_ACK:
        atp_cmd_ack(frame);
        break;
    case CMD_HEARTBEAT:
        atp_cmd_heartbeat(frame);
        break;
    case CMD_SET_AIRPLANE:
        atp_cmd_airplane(frame);
        LOG_D(TAG, "On frame got command -> [SET_AIRPLANE]");
        break;
    case CMD_SET_TRACKER:
        break;
    case CMD_SET_PARAM:
        LOG_I(TAG, "On frame got command -> [SET_PARAM]");
        atp_cmd_setparam(frame);
        break;
    case CMD_SET_HOME:
        atp_cmd_sethome(frame);
        LOG_I(TAG, "On frame got command -> [SET_AIRPLANE]");
        break;
    case CMD_GET_AIRPLANE:
    case CMD_GET_TRACKER:
    case CMD_GET_PARAM:
    case CMD_GET_HOME:
        LOG_I(TAG, "On frame got command -> %d", frame->atp_cmd);
        for (int i = 0; i < 5; i++)
        {
            if (atp->atp_cmd->cmds[i] == 0)
            {
                atp->atp_cmd->cmds[i] = frame->atp_cmd;
                break;
            }
        }
        break;
    case CMD_CONTROL:
        atp_cmd_control(frame);
        break;
    }
}

static void atp_frame_decode(void *t, void *data, int offset, int len)
{
    uint8_t *buffer = (uint8_t *)data;
    atp_t *atp = (atp_t *)t;
    atp->dec_frame->atp_status = IDLE;

    for (int i = offset; i < offset + len; i++)
    {
        switch (atp->dec_frame->atp_status)
        {
        case IDLE:
            if (buffer[i] == TP_PACKET_LEAD) 
            {
                atp->dec_frame->atp_status = STATE_LEAD;
            }
            //printf("ATP -> LEAD %#X | STATUS:%d\n", buffer[i], atp->dec_frame->atp_status);
            break;
        case STATE_LEAD:
            if (buffer[i] == TP_PACKET_START)
            {
                atp->dec_frame->atp_status = STATE_START;
            }
            else
            {
                atp->dec_frame->atp_status = IDLE;
            }
            //printf("ATP -> START %#X | STATUS:%d\n", buffer[i], atp->dec_frame->atp_status);
            break;
        case STATE_START:
            atp->dec_frame->atp_status = STATE_CMD;
            atp->dec_frame->atp_cmd = buffer[i];
            //printf("ATP -> CMD %#X | STATUS:%d\n", buffer[i], atp->dec_frame->atp_status);
            break;
        case STATE_CMD:
            atp->dec_frame->atp_status = STATE_INDEX;
            atp->dec_frame->atp_index = buffer[i];
            //printf("ATP -> INDEX %#X | STATUS:%d\n", buffer[i], atp->dec_frame->atp_status);
            break;
        case STATE_INDEX:
            atp->dec_frame->atp_status = STATE_LEN;
            atp->dec_frame->atp_tag_len = buffer[i];
            atp->dec_frame->atp_crc = buffer[i];
            //printf("ATP -> LEN %#X | STATUS:%d\n", buffer[i], atp->dec_frame->atp_status);
            if (atp->dec_frame->atp_tag_len == 0) atp->dec_frame->atp_status = STATE_DATA;
            break;
        case STATE_LEN:
            atp->dec_frame->atp_status = STATE_DATA;
            //printf("ATP -> DATA %#X | STATUS:%d | CRC:%d\n", buffer[i], atp->dec_frame->atp_status,  atp->dec_frame->atp_crc);
            atp->dec_frame->atp_crc ^= buffer[i];
            break;
        case STATE_DATA:
            //printf("ATP -> DATA %#X | STATUS:%d | CRC:%d INDEX:%d\n", buffer[i], atp->dec_frame->atp_status,  atp->dec_frame->atp_crc, atp->dec_frame->buffer_index);
            atp->dec_frame->atp_crc ^= buffer[i];
            if (atp->dec_frame->buffer_index - 4 == atp->dec_frame->atp_tag_len)
            {
                //printf("ATP -> DECODE DONE\n");
                if (atp->dec_frame->atp_crc == 0)
                {
                    atp->dec_frame->buffer_index = 5;
                    memcpy(atp->dec_frame->buffer, &buffer[offset], len);
                    atp_tag_analysis(atp->dec_frame);
                    // printf("ATP -> Success\n");
                }
                atp->dec_frame->atp_status = IDLE;
            }
            break;
        }

        atp->dec_frame->buffer_index = i - offset;
        //atp_dec_frame.atp_status = state;
    }
}

uint8_t *atp_frame_encode(void *data)
{
    atp_frame_t *frame = (atp_frame_t *)data;

    frame->buffer_index = 0;
    frame->buffer[frame->buffer_index++] = TP_PACKET_LEAD;              //lead
    frame->buffer[frame->buffer_index++] = TP_PACKET_START;             //start
    frame->buffer[frame->buffer_index++] = frame->atp_cmd;              //cmd
    frame->atp_index = frame->atp_index >= 0xff ? 0 : frame->atp_index + 1;
    frame->buffer[frame->buffer_index++] = frame->atp_index;            //index
    frame->buffer[frame->buffer_index++] = 0;                           //tag length

    switch (frame->atp_cmd)
    {
    case CMD_HEARTBEAT:
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_T_IP;
        tag_write_telemetry(frame, atp_get_telemetry_tag_val(TAG_TRACKER_T_IP));
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_T_PORT;
        tag_write_telemetry(frame, atp_get_telemetry_tag_val(TAG_TRACKER_T_PORT));
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_MODE;
        tag_write_telemetry(frame, atp_get_telemetry_tag_val(TAG_TRACKER_MODE));
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_FLAG;
        tag_write_telemetry(frame, atp_get_telemetry_tag_val(TAG_TRACKER_FLAG));
        break;
    case CMD_GET_PARAM:
        // tracker
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SHOW_COORDINATE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_SHOW_COORDINATE);
        // bettery
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_BATTERY_ENABLE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_ENABLE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_BATTERY_VOLTAGE_SCALE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_VOLTAGE_SCALE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_BATTERY_MAX_VOLTAGE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_MAX_VOLTAGE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_BATTERY_MIN_VOLTAGE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_MIN_VOLTAGE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_BATTERY_CENTER_VOLTAGE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_BATTERY_CENTER_VOLTAGE);
        // power
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_POWER_ENABLE;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_POWER_ENABLE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_MONITOR_POWER_ON;
        tag_write_setting(frame, SETTING_KEY_TRACKER_MONITOR_POWER_TURN);
        // wifi
        frame->buffer[frame->buffer_index++] = TAG_PARAM_WIFI_SSID;
        tag_write_setting(frame, SETTING_KEY_WIFI_SSID);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_WIFI_PWD;
        tag_write_setting(frame, SETTING_KEY_WIFI_PWD);
        // servo
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_COURSE;
        tag_write_setting(frame, SETTING_KEY_SERVO_COURSE);
        // servo pan
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_PAN_MIN_PLUSEWIDTH;
        tag_write_setting(frame, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_PAN_MAX_PLUSEWIDTH;
        tag_write_setting(frame, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_PAN_MAX_DEGREE;
        tag_write_setting(frame, SETTING_KEY_SERVO_PAN_MAX_DEGREE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_PAN_MIN_DEGREE;
        tag_write_setting(frame, SETTING_KEY_SERVO_PAN_MIN_DEGREE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_PAN_DIRECTION;
        tag_write_setting(frame, SETTING_KEY_SERVO_PAN_DIRECTION);
        // servo tilt
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_TILT_MIN_PLUSEWIDTH;
        tag_write_setting(frame, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_TILT_MAX_PLUSEWIDTH;
        tag_write_setting(frame, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_TILT_MAX_DEGREE;
        tag_write_setting(frame, SETTING_KEY_SERVO_TILT_MAX_DEGREE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_TILT_MIN_DEGREE;
        tag_write_setting(frame, SETTING_KEY_SERVO_TILT_MIN_DEGREE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_TILT_DIRECTION;
        tag_write_setting(frame, SETTING_KEY_SERVO_TILT_DIRECTION);
        // servo ease
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_EASE_OUT_TYPE;
        tag_write_setting(frame, SETTING_KEY_SERVO_EASE_OUT_TYPE);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_EASE_MAX_STEPS;
        tag_write_setting(frame, SETTING_KEY_SERVO_EASE_MAX_STEPS);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_EASE_MIN_PULSEWIDTH;
        tag_write_setting(frame, SETTING_KEY_SERVO_EASE_MIN_PULSEWIDTH);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_EASE_STEP_MS;
        tag_write_setting(frame, SETTING_KEY_SERVO_EASE_STEP_MS);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_EASE_MAX_MS;
        tag_write_setting(frame, SETTING_KEY_SERVO_EASE_MAX_MS);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SERVO_EASE_MIN_MS;
        tag_write_setting(frame, SETTING_KEY_SERVO_EASE_MIN_MS);
        // screen
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SCREEN_BRIGHTNESS;
        tag_write_setting(frame, SETTING_KEY_SCREEN_BRIGHTNESS);
        frame->buffer[frame->buffer_index++] = TAG_PARAM_SCREEN_AUTO_OFF;
        tag_write_setting(frame, SETTING_KEY_SCREEN_AUTO_OFF);
        // beeper
        frame->buffer[frame->buffer_index++] = TAG_PARAM_BEEPER_ENABLE;
        tag_write_setting(frame, SETTING_KEY_BEEPER_ENABLE);
        break;
    default:
        frame->buffer_index = 0;
        return frame->buffer;
    }
    frame->buffer[4] = frame->buffer_index - 5;

    frame->atp_crc = 0;
    for (int i = 4; i < frame->buffer_index; i++)
    {
        frame->atp_crc ^= frame->buffer[i];
    }
    
    frame->buffer[frame->buffer_index++] = frame->atp_crc;

    return frame->buffer;
}

void atp_init(atp_t *t)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    atp = t;
    t->atp_decode = atp_frame_decode;
    t->plane_vals = (telemetry_t *)&plane_vals;
    t->tracker_vals = (telemetry_t *)&tracker_vals;
    t->param_vals = (telemetry_t *)&param_vals;
    // t->iats_pro_param_vals = (telemetry_t *)&iats_pro_param_vals;
    t->atp_cmd = &atp_cmd;
    t->atp_ctr = &atp_ctr;
    t->dec_frame = (atp_frame_t *)malloc(sizeof(atp_frame_t));
    t->enc_frame = (atp_frame_t *)malloc(sizeof(atp_frame_t));

    for(int i = 0; i < TAG_PLANE_COUNT + TAG_TRACKER_COUNT + TAG_PARAM_COUNT; i++)
    {
        telemetry_t *val = atp_get_telemetry_tag_val(atp_tag_infos[i].tag);
        val->type = atp_tag_infos[i].type;
    }
}

uint8_t atp_get_telemetry_tag_index(uint8_t tag)
{
    if (tag >= TAG_PLANE_MASK && tag < (TAG_PLANE_MASK + TAG_PLANE_COUNT))
    {
        return tag ^ TAG_PLANE_MASK;
    }
    else if (tag >= TAG_TRACKER_MASK && tag < (TAG_TRACKER_MASK + TAG_TRACKER_COUNT))
    {
        return tag ^ TAG_TRACKER_MASK;
    }
    else if (tag >= TAG_PARAM_MASK && tag < (TAG_PARAM_MASK + TAG_PARAM_COUNT))
    {
        return tag ^ TAG_PARAM_MASK;
    }    

    return 0;
}

telemetry_t *atp_get_telemetry_tag_val(uint8_t tag)
{
    if (tag >= TAG_PLANE_MASK && tag < (TAG_PLANE_MASK + TAG_PLANE_COUNT))
    {
        return &plane_vals[tag ^ TAG_PLANE_MASK];
    }
    else if (tag >= TAG_TRACKER_MASK && tag < (TAG_TRACKER_MASK + TAG_TRACKER_COUNT))
    {
        return &tracker_vals[tag ^ TAG_TRACKER_MASK];
    }
    else if (tag >= TAG_PARAM_MASK && tag < (TAG_PARAM_MASK + TAG_PARAM_COUNT))
    {
        return &param_vals[tag ^ TAG_PARAM_MASK];
    }

    return &plane_vals[0];
}

uint8_t atp_popup_cmd()
{
    int index = 0;
    uint8_t ret = atp_cmd.cmds[index];
    uint8_t cmd = ret;

    while (cmd == 0 && index < 4)
    {
        atp_cmd.cmds[index] = atp_cmd.cmds[index + 1];
        index++;
        cmd = atp_cmd.cmds[index];
    }

    atp_cmd.cmds[index] = 0;  

    return ret;  
}

void atp_remove_ctr(uint8_t len)
{
    int index = 0;
    uint8_t ctr = atp_ctr.ctrs[index];

    LOG_I(TAG, "Remove [%d] command from stack.", ctr);

    while (ctr == 0 && index < 4)
    {
        atp_ctr.ctrs[index] = atp_ctr.ctrs[index + 1];
        index++;
        ctr = atp_ctr.ctrs[index];
    }

    atp_ctr.ctrs[index] = 0;  
    atp_ctr.data_index -= len;
}