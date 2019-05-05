#include <hal/log.h>

#include "atp.h"
#include "tracker/tracker.h"

// static const char *TAG = "atp";
static atp_frame_t atp_frame;
static tracker_t *tracker;

static telemetry_t plane_tags[TAG_PLANE_COUNT];
static telemetry_t home_tags[TAG_HOME_COUNT];
static telemetry_t param_tags[TAG_PARAM_COUNT];

static const atp_tag_info_t atp_tag_infos[] = {
    {TAG_PLANE_LONGITUDE,           TELEMETRY_TYPE_INT32,   "Lon",      telemetry_format_coordinate},                 
    {TAG_PLANE_LATITUDE,            TELEMETRY_TYPE_INT32,   "Lat",      telemetry_format_coordinate},
    {TAG_PLANE_ALTITUDE,            TELEMETRY_TYPE_INT16,   "Alt",      telemetry_format_att},
    {TAG_PLANE_SPEED,               TELEMETRY_TYPE_INT16,   "Spd",      telemetry_format_horizontal_speed},
    {TAG_PLANE_DISTANCE,            TELEMETRY_TYPE_UINT32,  "Dist",     telemetry_format_metre},
    {TAG_PLANE_STAR,                TELEMETRY_TYPE_INT16,   "GPS Sta.", telemetry_format_u8},
    {TAG_PLANE_FIX,                 TELEMETRY_TYPE_UINT8,   "GPS Fix.", telemetry_format_gps_fix},
    {TAG_PLANE_PITCH,               TELEMETRY_TYPE_INT16,   "Pitch",    telemetry_format_deg},
    {TAG_PLANE_ROLL,                TELEMETRY_TYPE_INT16,   "Roll",     telemetry_format_deg},
    {TAG_PLANE_HEADING,             TELEMETRY_TYPE_UINT16,  "Heading",  telemetry_format_deg},
    {TAG_HOME_LONGITUDE,            TELEMETRY_TYPE_INT32,   "Lon",      telemetry_format_deg},
    {TAG_HOME_LATITUDE,             TELEMETRY_TYPE_INT32,   "Lat",      telemetry_format_coordinate},
    {TAG_HOME_ALTITUDE,             TELEMETRY_TYPE_INT16,   "Alt",      telemetry_format_coordinate},
    {TAG_HOME_HEADING,              TELEMETRY_TYPE_UINT16,  "Heading",  telemetry_format_deg},
    {TAG_HOME_PITCH,                TELEMETRY_TYPE_INT16,   "Pitch",    telemetry_format_deg},
    {TAG_HOME_VOLTAGE,              TELEMETRY_TYPE_UINT16,  "Batt. V.", telemetry_format_voltage},
    {TAG_HOME_MODE,                 TELEMETRY_TYPE_UINT8,   "Mode",     telemetry_format_tracker_mode},
    {TAG_HOME_DECLINATION,          TELEMETRY_TYPE_INT8,    "Dec.",     telemetry_format_deg},
    {TAG_PARAM_PID_P,               TELEMETRY_TYPE_UINT16,  "P",        telemetry_format_u16},
    {TAG_PARAM_PID_I,               TELEMETRY_TYPE_UINT16,  "I",        telemetry_format_u16},
    {TAG_PARAM_PID_D,               TELEMETRY_TYPE_UINT16,  "D",        telemetry_format_u16},
    {TAG_PARAM_TITL_0,              TELEMETRY_TYPE_UINT16,  "Titl 0",   telemetry_format_u16},
    {TAG_PARAM_TITL_90,             TELEMETRY_TYPE_UINT16,  "Titl 90",  telemetry_format_u16},
    {TAG_PARAM_PAN_0,               TELEMETRY_TYPE_UINT16,  "Pan 0",    telemetry_format_u16},
    {TAG_PARAM_OFFSET,              TELEMETRY_TYPE_INT16,   "Offset",   telemetry_format_deg},
    {TAG_PARAM_TRACKING_DISTANCE,   TELEMETRY_TYPE_UINT8,   "Start m.", telemetry_format_metre},
    {TAG_PARAM_MAX_PID_ERROR,       TELEMETRY_TYPE_UINT16,  "PID err.", telemetry_format_u16},
    {TAG_PARAM_MIN_PAN_SPEED,       TELEMETRY_TYPE_UINT8,   "Pan spd.", telemetry_format_u8},
    {TAG_PARAM_DECLINATION,         TELEMETRY_TYPE_INT8,    "Decl.",    telemetry_format_deg},
};

static uint8_t tagread_u8(atp_frame_t *frame)
{
    return frame->atp_tags[frame->atp_tag_index++];
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

static void atp_cmd_heartbeat(atp_frame_t *frame)
{
    while (frame->atp_tag_index < frame->atp_tag_len)
    {

        switch (frame->atp_tags[frame->atp_tag_index++])
        {
        case TAG_BASE_ACK:
            frame->atp_tag_index++;
            tracker->internal.status = tagread_u8(frame);
            if (tracker->internal.status >= 3)
                tracker->last_heartbeat = time_millis_now();
            break;
        default:
            frame->atp_tag_index++;
            break;
        }
    }
}

static void atp_cmd_airplane(atp_frame_t *frame)
{
    time_micros_t now = time_micros_now();

    while (frame->atp_tag_index < frame->atp_tag_len)
    {
        switch (frame->atp_tags[frame->atp_tag_index++])
        {
        case TAG_PLANE_LONGITUDE:   //plane's longitude L:4
            frame->atp_tag_index++;
            ATP_SET_I32(TAG_PLANE_LATITUDE, (int32_t)tagread_u32(frame), now);
            if (!(tracker->internal.flag & TRACKER_FLAG_PLANESETED)) tracker->internal.flag_changed(tracker, TRACKER_FLAG_PLANESETED);
            break;
        case TAG_PLANE_LATITUDE:    //plane's latitude L:4
            frame->atp_tag_index++;
            ATP_SET_I32(TAG_PLANE_LATITUDE, (int32_t)tagread_u32(frame), now);
            if (!(tracker->internal.flag & TRACKER_FLAG_PLANESETED)) tracker->internal.flag_changed(tracker, TRACKER_FLAG_PLANESETED);
            break;
        case TAG_PLANE_ALTITUDE:    //plane's altitude L:4
            frame->atp_tag_index++;
            ATP_SET_I16(TAG_PLANE_ALTITUDE, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_SPEED:       //plane's speed L:2
            frame->atp_tag_index++;
            ATP_SET_I16(TAG_PLANE_SPEED, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_DISTANCE:    //plane's distance L:4
            frame->atp_tag_index++;
            ATP_SET_U32(TAG_PLANE_DISTANCE, tagread_u32(frame), now);
            break;
        case TAG_PLANE_STAR:        //plane's star numbers L:1
            frame->atp_tag_index++;
            ATP_SET_U8(TAG_PLANE_STAR, tagread_u8(frame), now);
            break;
        case TAG_PLANE_FIX:         //plane's fix type L:1
            frame->atp_tag_index++;
            ATP_SET_U8(TAG_PLANE_FIX, tagread_u8(frame), now);
            break;
        case TAG_PLANE_PITCH:       //plane's pitch L:2
            frame->atp_tag_index++;
            ATP_SET_I16(TAG_PLANE_PITCH, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_ROLL:        //plane's roll L:2
            frame->atp_tag_index++;
            ATP_SET_I16(TAG_PLANE_ROLL, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_PLANE_HEADING:     //plane's heading L:2
            frame->atp_tag_index++;
            ATP_SET_U16(TAG_PLANE_HEADING, tagread_u16(frame), now);
            break;
        default:
            frame->atp_tag_index++;
            break;
        }
    }
}

static void atp_cmd_sethome(atp_frame_t *frame)
{
    time_micros_t now = time_micros_now();

    while (frame->atp_tag_index < frame->atp_tag_len)
    {
        switch (frame->atp_tags[frame->atp_tag_index++])
        {
        case TAG_HOME_LONGITUDE: //home's longitude L:4
            frame->atp_tag_index++;
            ATP_SET_I32(TAG_HOME_LONGITUDE, (int32_t)tagread_u32(frame), now);
            if (!(tracker->internal.flag & TRACKER_FLAG_HOMESETED)) tracker->internal.flag_changed(tracker, TRACKER_FLAG_HOMESETED);
        case TAG_HOME_LATITUDE: //home's latitude L:4
            frame->atp_tag_index++;
            ATP_SET_I32(TAG_HOME_LATITUDE, (int32_t)tagread_u32(frame), now);
            if (!(tracker->internal.flag & TRACKER_FLAG_HOMESETED)) tracker->internal.flag_changed(tracker, TRACKER_FLAG_HOMESETED);
            break;
        case TAG_HOME_ALTITUDE: //home's altitude L:2
            frame->atp_tag_index++;
            ATP_SET_I16(TAG_HOME_ALTITUDE, (int16_t)tagread_u16(frame), now);
            break;
        case TAG_HOME_MODE: //home's mode L:1
            frame->atp_tag_index++;
            uint8_t mode = tagread_u8(frame);
            ATP_SET_U8(TAG_HOME_MODE, tagread_u8(frame), now);
            tracker->internal.status = mode;
            break;
        case TAG_HOME_DECLINATION: //home's declination L:1
            frame->atp_tag_index++;
            ATP_SET_I8(TAG_HOME_DECLINATION, (int8_t)tagread_u8(frame), now);
            break;
        default:
            frame->atp_tag_index++;
            break;
        }
    }
}

static void atp_cmd_setparam(atp_frame_t *frame)
{
    while (frame->atp_tag_index < frame->atp_tag_len)
    {
        switch (frame->atp_tags[frame->atp_tag_index++])
        {
        case TAG_PARAM_PID_P: //PID_P L:2
            frame->atp_tag_index++;
            // PTParam->pid_p = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pid_p, PARAM_EPPROM_POS_PID_P);
            break;
        case TAG_PARAM_PID_I: //PID_I L:2
            frame->atp_tag_index++;
            // PTParam->pid_i = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pid_i, PARAM_EPPROM_POS_PID_I);
            break;
        case TAG_PARAM_PID_D: //PID_D L:2
            frame->atp_tag_index++;
            // PTParam->pid_d = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pid_d, PARAM_EPPROM_POS_PID_D);
            break;
        case TAG_PARAM_TITL_0: //俯仰零度 L:2
            frame->atp_tag_index++;
            // PTParam->tilt_0 = tagread_u16();
            // StoreToEEPROM_u16(PTParam->tilt_0, PARAM_EPPROM_POS_TILT_0);
            break;
        case TAG_PARAM_TITL_90: //俯仰90度 L:2
            frame->atp_tag_index++;
            // PTParam->tilt_90 = tagread_u16();
            // StoreToEEPROM_u16(PTParam->tilt_90, PARAM_EPPROM_POS_TILT_90);
            break;
        case TAG_PARAM_PAN_0: //水平中立点 L:2
            frame->atp_tag_index++;
            // PTParam->pan_center = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pan_center, PARAM_EPPROM_POS_PAN_CENTER);
            break;
        case TAG_PARAM_OFFSET: //罗盘偏移量 L:2
            frame->atp_tag_index++;
            // PTParam->compass_offset = (int8_t)tagread_u16();
            // StoreToEEPROM_u16(PTParam->compass_offset, PARAM_EPPROM_POS_COMPASS_OFFSET);
            break;
        case TAG_PARAM_TRACKING_DISTANCE: //开始跟踪距离 L:1
            frame->atp_tag_index++;
            // PTParam->start_tracking_distance = tagread_u8();
            // StoreToEEPROM_u8(PTParam->start_tracking_distance, PARAM_EPPROM_POS_START_TRACKING_DISTANCE);
            break;
        case TAG_PARAM_MAX_PID_ERROR: //最大角度偏移 L:1
            frame->atp_tag_index++;
            // PTParam->pid_max_error = tagread_u8();
            // StoreToEEPROM_u8(PTParam->pid_max_error, PARAM_EPPROM_POS_PID_MAX_ERROR);
            break;
        case TAG_PARAM_MIN_PAN_SPEED: //最小水平舵机速度 L:1
            frame->atp_tag_index++;
            // PTParam->pan_min_speed = tagread_u8();
            // StoreToEEPROM_u8(PTParam->pan_min_speed, PARAM_EPPROM_POS_PAN_MIN_SPEED);
            break;
        case TAG_PARAM_DECLINATION: //磁偏角 L:1
            frame->atp_tag_index++;
            // PTParam->compass_declination = (int8_t)tagread_u8();
            // StoreToEEPROM_u8((uint8_t)PTParam->compass_declination, PARAM_EPPROM_POS_COMPASS_DECLINATION);
            break;
        default:
            frame->atp_tag_index++;
            break;
        }
    }
}

static void atp_cmd_control(atp_frame_t *frame)
{
    while (frame->atp_tag_index < frame->atp_tag_len)
    {
        switch (frame->atp_tags[atp_frame.atp_tag_index++])
        {
        case TAG_CTR_MODE: //设置模式 L:1
            frame->atp_tag_index++;
            // PTracker->mode = tagread_u8();
            // setReceiveCmd(TAG_CTR_MODE);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_MODE;
            break;
        case TAG_CTR_AUTO_POINT_TO_NORTH: //设置自动指北 L:1
            frame->atp_tag_index++;
            // AUTO_POINT_TO_NORTH = tagread_u8();
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_AUTO_POINT_TO_NORTH;
            break;
        case TAG_CTR_CALIBRATE: //校准 L:1
            frame->atp_tag_index++;
            // uint8_t cal;
            // cal = tagread_u8();
            // setReceiveCmd(TAG_CTR_CALIBRATE);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_CALIBRATE;
            break;
        case TAG_CTR_HEADING: //指向 L:2
            frame->atp_tag_index++;
            // PTracker->course = tagread_u16();
            // setReceiveCmd(TAG_CTR_HEADING);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_HEADING;
            break;
        case TAG_CTR_TILT: //俯仰 L:1
            frame->atp_tag_index++;
            // PTracker->pitching = tagread_u8();
            // setReceiveCmd(TAG_CTR_TILT);
            // Receive_Cmd_Ctr_Tag[Receive_Cmd_Ctr_Tag_Count++] = TAG_CTR_TILT;
            break;
        default:
            frame->atp_tag_index++;
            break;
        }
    }
}

static void atp_tag_analysis(atp_frame_t *frame)
{
    switch (frame->atp_cmd)
    {
    case CMD_ACK:
        break;
    case CMD_HEARTBEAT:
        atp_cmd_heartbeat(frame);
        break;
    case CMD_SET_AIRPLANE:
        atp_cmd_airplane(frame);
        break;
    case CMD_SET_TRACKER:
        break;
    case CMD_SET_PARAM:
        atp_cmd_setparam(frame);
        break;
    case CMD_SET_HOME:
        atp_cmd_sethome(frame);
        break;
    case CMD_CONTROL:
        atp_cmd_control(frame);
        break;
    }
}

static void atp_frame_decode(void *data, int offset, int len)
{
    uint8_t *buffer = (uint8_t *)data;
    atp_frame_status_e state = IDLE;

    for (int i = offset; i < offset + len; i++)
    {
        state = IDLE;
        switch (atp_frame.atp_status)
        {
        case IDLE:
            if (buffer[i] == TP_PACKET_LEAD)
                state = STATE_LEAD;
            break;
        case STATE_LEAD:
            if (buffer[i] == TP_PACKET_START)
                state = STATE_START;
            break;
        case STATE_START:
            if (buffer[i] == STATE_START)
            {
                state = STATE_CMD;
                atp_frame.atp_cmd = buffer[i];
            }
            break;
        case STATE_CMD:
            state = STATE_INDEX;
            atp_frame.atp_index = buffer[i];
            break;
        case STATE_INDEX:
            state = STATE_LEN;
            atp_frame.atp_tag_len = buffer[i];
            break;
        case STATE_LEN:
            state = STATE_DATA;
            atp_frame.atp_tag_index = 0;
            atp_frame.atp_crc = buffer[i];
            atp_frame.atp_tags[atp_frame.atp_tag_index++] = buffer[i];
            break;
        case STATE_DATA:
            atp_frame.atp_crc ^= buffer[i];
            if (atp_frame.atp_tag_index == atp_frame.atp_tag_len)
            {
                if (atp_frame.atp_crc == 0)
                {
                    atp_frame.atp_tag_index = 0;
                    atp_tag_analysis(&atp_frame);
                }
                state = IDLE;
            }
            else
            {
                atp_frame.atp_tags[atp_frame.atp_tag_index++] = buffer[i];
            }
            break;
        }

        atp_frame.atp_status = state;
    }
}

void atp_init(tracker_t *t)
{
    tracker = t;
    // tracker->wifi->callback = atp_frame_decode;
    tracker->internal.atp_decode = atp_frame_decode;
    tracker->home_tags = (telemetry_t *)&home_tags;
    tracker->plane_tags = (telemetry_t *)&plane_tags;
    tracker->param_tags = (telemetry_t *)&param_tags;
}

uint8_t atp_get_tag_index(uint8_t tag)
{
    if (tag >= TAG_PLANE_MASK && tag < (TAG_PLANE_MASK + TAG_PLANE_COUNT))
    {
        return tag ^ TAG_PLANE_MASK;
    }
    else if (tag >= TAG_HOME_MASK && tag < (TAG_HOME_MASK + TAG_HOME_COUNT))
    {
        return tag ^ TAG_HOME_MASK;
    }
    else if (tag >= TAG_PARAM_MASK && tag < (TAG_PARAM_MASK + TAG_PARAM_COUNT))
    {
        return tag ^ TAG_PARAM_MASK;
    }

    return 0;
}

telemetry_t *atp_get_tag_val(uint8_t tag)
{
    if (tag >= TAG_PLANE_MASK && tag < (TAG_PLANE_MASK + TAG_PLANE_COUNT))
    {
        return &plane_tags[tag ^ TAG_PLANE_MASK];
    }
    else if (tag >= TAG_HOME_MASK && tag < (TAG_HOME_MASK + TAG_HOME_COUNT))
    {
        return &home_tags[tag ^ TAG_HOME_MASK];
    }
    else if (tag >= TAG_PARAM_MASK && tag < (TAG_PARAM_MASK + TAG_PARAM_COUNT))
    {
        return &param_tags[tag ^ TAG_PARAM_MASK];
    }

    return &plane_tags[0];
}