#include <hal/log.h>
#include <string.h>
#include "atp.h"
#include "tracker/observer.h"

// static const char *TAG = "atp";
//static atp_frame_t atp_dec_frame;
//static atp_frame_t atp_enc_frame;
static telemetry_t plane_vals[TAG_PLANE_COUNT];
static telemetry_t tracker_vals[TAG_TRACKER_COUNT];
static telemetry_t param_vals[TAG_PARAM_COUNT];
static atp_t *atp;

//static uint8_t dec_buff[512];
//static uint8_t enc_buff[512];

// pTr_flag_change flag_change;
// void *tracker;

static const atp_tag_info_t atp_tag_infos[] = {
    {TAG_PLANE_LONGITUDE,           TELEMETRY_TYPE_INT32,   "Lon",      telemetry_format_coordinate},                 
    {TAG_PLANE_LATITUDE,            TELEMETRY_TYPE_INT32,   "Lat",      telemetry_format_coordinate},
    {TAG_PLANE_ALTITUDE,            TELEMETRY_TYPE_INT32,   "Alt",      telemetry_format_altitude},
    {TAG_PLANE_SPEED,               TELEMETRY_TYPE_INT16,   "Spd",      telemetry_format_horizontal_speed},
    {TAG_PLANE_DISTANCE,            TELEMETRY_TYPE_UINT32,  "Dist",     telemetry_format_metre},
    {TAG_PLANE_STAR,                TELEMETRY_TYPE_INT16,   "GPS Sta.", telemetry_format_u8},
    {TAG_PLANE_FIX,                 TELEMETRY_TYPE_UINT8,   "GPS Fix.", telemetry_format_gps_fix},
    {TAG_PLANE_PITCH,               TELEMETRY_TYPE_INT16,   "Pitch",    telemetry_format_deg},
    {TAG_PLANE_ROLL,                TELEMETRY_TYPE_INT16,   "Roll",     telemetry_format_deg},
    {TAG_PLANE_HEADING,             TELEMETRY_TYPE_UINT16,  "Heading",  telemetry_format_deg},
    {TAG_TRACKER_LONGITUDE,         TELEMETRY_TYPE_INT32,   "Lon",      telemetry_format_coordinate},
    {TAG_TRACKER_LATITUDE,          TELEMETRY_TYPE_INT32,   "Lat",      telemetry_format_coordinate},
    {TAG_TRACKER_ALTITUDE,          TELEMETRY_TYPE_INT32,   "Alt",      telemetry_format_altitude},
    {TAG_TRACKER_HEADING,           TELEMETRY_TYPE_UINT16,  "Heading",  telemetry_format_deg},
    {TAG_TRACKER_PITCH,             TELEMETRY_TYPE_INT16,   "Pitch",    telemetry_format_deg},
    {TAG_TRACKER_VOLTAGE,           TELEMETRY_TYPE_UINT16,  "Batt. V.", telemetry_format_voltage},
    {TAG_TRACKER_MODE,              TELEMETRY_TYPE_UINT8,   "Mode",     telemetry_format_tracker_mode},
    {TAG_TRACKER_DECLINATION,       TELEMETRY_TYPE_INT8,    "Dec.",     telemetry_format_deg},
    {TAG_TRACKER_T_IP,              TELEMETRY_TYPE_UINT32,  "T IP.",    telemetry_format_ip},
    {TAG_TRACKER_T_PORT,            TELEMETRY_TYPE_UINT16,  "T Port.",  telemetry_format_u16},
    {TAG_TRACKER_S_IP,              TELEMETRY_TYPE_UINT32,  "S IP.",    telemetry_format_ip},
    {TAG_TRACKER_S_PORT,            TELEMETRY_TYPE_UINT16,  "S Port.",  telemetry_format_u16},
    {TAG_TRACKER_FLAG,              TELEMETRY_TYPE_UINT8,   "Flag",     telemetry_format_u8},
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

static void tag_write(atp_frame_t *frame, telemetry_t *val)
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
        case TELEMETRY_TYPE_STRING:

            break;
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
            // printf("TAG_PLANE_LONGITUDE:%d\n", telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LONGITUDE)));
            break;
        case TAG_PLANE_LATITUDE:    //plane's latitude L:4
            frame->buffer_index++;
            ATP_SET_I32(TAG_PLANE_LATITUDE, (int32_t)tagread_u32(frame), now);
            atp->tag_value_changed(atp->tracker, TAG_PLANE_LATITUDE);
            // printf("TAG_PLANE_LATITUDE:%d\n", telemetry_get_i32(atp_get_tag_val(TAG_PLANE_LATITUDE)));
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
            ATP_SET_I32(TAG_TRACKER_LONGITUDE, (int32_t)tagread_u32(frame), now);
            atp->tag_value_changed(atp->tracker, TAG_TRACKER_LONGITUDE);
            // printf("TAG_TRACKER_LONGITUDE:%d\n", telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_LONGITUDE)));
            break;
        case TAG_TRACKER_LATITUDE: //tarcker's latitude L:4
            frame->buffer_index++;
            ATP_SET_I32(TAG_TRACKER_LATITUDE, (int32_t)tagread_u32(frame), now);
            atp->tag_value_changed(atp->tracker, TAG_TRACKER_LATITUDE);
            // printf("TAG_TRACKER_LATITUDE:%d\n", telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_LATITUDE)));
            break;
        case TAG_TRACKER_ALTITUDE: //tarcker's altitude L:4
            frame->buffer_index++;
            ATP_SET_I32(TAG_TRACKER_ALTITUDE, (int32_t)tagread_u32(frame), now);
            atp->tag_value_changed(atp->tracker, TAG_TRACKER_ALTITUDE);
            // printf("TAG_TRACKER_ALTITUDE:%d\n", telemetry_get_i32(atp_get_tag_val(TAG_TRACKER_ALTITUDE)));
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
        case TAG_PARAM_PID_P: //PID_P L:2
            frame->buffer_index++;
            // PTParam->pid_p = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pid_p, PARAM_EPPROM_POS_PID_P);
            break;
        case TAG_PARAM_PID_I: //PID_I L:2
            frame->buffer_index++;
            // PTParam->pid_i = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pid_i, PARAM_EPPROM_POS_PID_I);
            break;
        case TAG_PARAM_PID_D: //PID_D L:2
            frame->buffer_index++;
            // PTParam->pid_d = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pid_d, PARAM_EPPROM_POS_PID_D);
            break;
        case TAG_PARAM_TITL_0: //俯仰零度 L:2
            frame->buffer_index++;
            // PTParam->tilt_0 = tagread_u16();
            // StoreToEEPROM_u16(PTParam->tilt_0, PARAM_EPPROM_POS_TILT_0);
            break;
        case TAG_PARAM_TITL_90: //俯仰90度 L:2
            frame->buffer_index++;
            // PTParam->tilt_90 = tagread_u16();
            // StoreToEEPROM_u16(PTParam->tilt_90, PARAM_EPPROM_POS_TILT_90);
            break;
        case TAG_PARAM_PAN_0: //水平中立点 L:2
            frame->buffer_index++;
            // PTParam->pan_center = tagread_u16();
            // StoreToEEPROM_u16(PTParam->pan_center, PARAM_EPPROM_POS_PAN_CENTER);
            break;
        case TAG_PARAM_OFFSET: //罗盘偏移量 L:2
            frame->buffer_index++;
            // PTParam->compass_offset = (int8_t)tagread_u16();
            // StoreToEEPROM_u16(PTParam->compass_offset, PARAM_EPPROM_POS_COMPASS_OFFSET);
            break;
        case TAG_PARAM_TRACKING_DISTANCE: //开始跟踪距离 L:1
            frame->buffer_index++;
            // PTParam->start_tracking_distance = tagread_u8();
            // StoreToEEPROM_u8(PTParam->start_tracking_distance, PARAM_EPPROM_POS_START_TRACKING_DISTANCE);
            break;
        case TAG_PARAM_MAX_PID_ERROR: //最大角度偏移 L:1
            frame->buffer_index++;
            // PTParam->pid_max_error = tagread_u8();
            // StoreToEEPROM_u8(PTParam->pid_max_error, PARAM_EPPROM_POS_PID_MAX_ERROR);
            break;
        case TAG_PARAM_MIN_PAN_SPEED: //最小水平舵机速度 L:1
            frame->buffer_index++;
            // PTParam->pan_min_speed = tagread_u8();
            // StoreToEEPROM_u8(PTParam->pan_min_speed, PARAM_EPPROM_POS_PAN_MIN_SPEED);
            break;
        case TAG_PARAM_DECLINATION: //磁偏角 L:1
            frame->buffer_index++;
            // PTParam->compass_declination = (int8_t)tagread_u8();
            // StoreToEEPROM_u8((uint8_t)PTParam->compass_declination, PARAM_EPPROM_POS_COMPASS_DECLINATION);
            break;
        default:
            frame->buffer_index++;
            break;
        }
    }
}

static void atp_cmd_control(atp_frame_t *frame)
{
    while (frame->buffer_index < frame->atp_tag_len)
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

static void atp_frame_decode(void *t, void *data, int offset, int len)
{
    uint8_t *buffer = (uint8_t *)data;
    atp_t *atp = (atp_t *)t;
    atp->dec_frame->atp_status = IDLE;
    // printf("ATP -> OFFSET:%d | LEN:%d\n", offset, len);

    for (int i = offset; i < offset + len; i++)
    {
        switch (atp->dec_frame->atp_status)
        {
        case IDLE:
            if (buffer[i] == TP_PACKET_LEAD) 
            {
                atp->dec_frame->atp_status = STATE_LEAD;
            }
            // printf("ATP -> LEAD %#X | STATUS:%d\n", buffer[i], atp_dec_frame.atp_status);
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
            // printf("ATP -> START %#X | STATUS:%d\n", buffer[i], atp_dec_frame.atp_status);
            break;
        case STATE_START:
            atp->dec_frame->atp_status = STATE_CMD;
            atp->dec_frame->atp_cmd = buffer[i];
            // printf("ATP -> CMD %#X | STATUS:%d\n", buffer[i], atp_dec_frame.atp_status);
            break;
        case STATE_CMD:
            atp->dec_frame->atp_status = STATE_INDEX;
            atp->dec_frame->atp_index = buffer[i];
            // printf("ATP -> INDEX %#X | STATUS:%d\n", buffer[i], atp_dec_frame.atp_status);
            break;
        case STATE_INDEX:
            atp->dec_frame->atp_status = STATE_LEN;
            atp->dec_frame->atp_tag_len = buffer[i];
            atp->dec_frame->atp_crc = buffer[i];
            // printf("ATP -> LEN %#X | STATUS:%d\n", buffer[i], atp_dec_frame.atp_status);
            break;
        case STATE_LEN:
            atp->dec_frame->atp_status = STATE_DATA;
            // printf("ATP -> DATA %#X | STATUS:%d | CRC:%d\n", buffer[i], atp_dec_frame.atp_status,  atp_dec_frame.atp_crc);
            atp->dec_frame->atp_crc ^= buffer[i];
            break;
        case STATE_DATA:
            // printf("ATP -> DATA %#X | STATUS:%d | CRC:%d INDEX:%d\n", buffer[i], atp_dec_frame.atp_status,  atp_dec_frame.atp_crc, atp_dec_frame.buffer_index);
            atp->dec_frame->atp_crc ^= buffer[i];
            if (atp->dec_frame->buffer_index - 4 == atp->dec_frame->atp_tag_len)
            {
                // printf("ATP -> DECODE DONE\n");
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
        tag_write(frame, atp_get_tag_val(TAG_TRACKER_T_IP));
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_T_PORT;
        tag_write(frame, atp_get_tag_val(TAG_TRACKER_T_PORT));
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_MODE;
        tag_write(frame, atp_get_tag_val(TAG_TRACKER_MODE));
        frame->buffer[frame->buffer_index++] = TAG_TRACKER_FLAG;
        tag_write(frame, atp_get_tag_val(TAG_TRACKER_FLAG));
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
    atp = t;
    t->atp_decode = atp_frame_decode;
    t->plane_vals = (telemetry_t *)&plane_vals;
    t->tracker_vals = (telemetry_t *)&tracker_vals;
    t->param_vals = (telemetry_t *)&param_vals;
    t->dec_frame = (atp_frame_t *)malloc(sizeof(atp_frame_t));
    t->enc_frame = (atp_frame_t *)malloc(sizeof(atp_frame_t));

    for(int i = 0; i < TAG_PLANE_COUNT + TAG_TRACKER_COUNT + TAG_PARAM_COUNT; i++)
    {
        telemetry_t *val = atp_get_tag_val(atp_tag_infos[i].tag);
        val->type = atp_tag_infos[i].type;
    }
}

uint8_t atp_get_tag_index(uint8_t tag)
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

telemetry_t *atp_get_tag_val(uint8_t tag)
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