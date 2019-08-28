#include <stdint.h>

#include "util/macros.h"
#include "util/time.h"
#include "util/data_state.h"
#include "tracker/telemetry.h"

#define MAX_TAG_COUNT							   10        //每帧数据最大TAG数
#define MAX_CMD_COUNT							   5         //最大缓存等待发送的指令数
   
#define TP_PACKET_LEAD							   0x24      //引导码 $
#define TP_PACKET_START							   0x54      //协议头 T
   
#define TAG_COUNT TAG_BASE_COUNT + TAG_PLANE_COUNT + TAG_TRACKER_COUNT + TAG_PARAM_COUNT + TAG_PARAM_IATS_PRO_COUNT  //TAG数量，定义了新的TAG需要增加这个值
#define TAG_BASE_COUNT                             3         //基础Tag数量
#define TAG_PLANE_COUNT                            10        //Plane tags count
#define TAG_TRACKER_COUNT                          13        //Tarcker tags count
#define TAG_PARAM_COUNT                            11        //Parameter tags count
#define TAG_PARAM_IATS_PRO_COUNT                   29        //iats_pro Parameter tags count
   
#define TAG_PLANE_MASK                             0x10      //Plane tags mask
#define TAG_TRACKER_MASK                           0x40      //Tarcker tags mask
#define TAG_PARAM_MASK                             0x70      //Parameter tags mask
#define TAG_PARAM_IATS_PRO_MASK                    0x80      //Parameter tags mask
   
//-----------------基础协议---------------------------------------------------
#define TAG_BASE_ACK							   0x00      //应答结果 L:1 V:0 失败 非0成功，成功应答命令的INDEX
#define TAG_BASE_HEARTBEAT						   0x01	  //设备心跳 L:1 V:0空闲 1正在跟踪 2调试模式 3手动模式
#define TAG_BASE_QUERY							   0x02	  //请求数据 L:1 V:CMD 请求的指令的CMD值
//-----------------飞机数据---------------------------------------------------
#define TAG_PLANE_LONGITUDE						   0x10      //飞机经度 L:4 
#define TAG_PLANE_LATITUDE						   0x11      //飞机纬度 L:4
#define TAG_PLANE_ALTITUDE						   0x12      //飞机高度 L:4
#define TAG_PLANE_SPEED							   0x13      //飞机速度 L:2
#define TAG_PLANE_DISTANCE						   0x14      //飞机距离 L:4
#define TAG_PLANE_STAR							   0x15      //定位星数 L:1
#define TAG_PLANE_FIX							   0x16      //定位类型 L:1
#define TAG_PLANE_PITCH							   0x17      //俯仰角度 L:2
#define TAG_PLANE_ROLL							   0x18      //横滚角度 L:2
#define TAG_PLANE_HEADING						   0x19      //飞机方向 L:2
//-----------------云台数据---------------------------------------------------
#define TAG_TRACKER_LONGITUDE					   0x40      //家的经度 L:4
#define TAG_TRACKER_LATITUDE					   0x41      //家的纬度 L:4
#define TAG_TRACKER_ALTITUDE					   0x42      //家的高度 L:4
#define TAG_TRACKER_HEADING						   0x43      //家的朝向 L:2
#define TAG_TRACKER_PITCH  						   0x44      //家的俯仰 L:1
#define TAG_TRACKER_VOLTAGE						   0x45      //家的电压 L:2
#define TAG_TRACKER_MODE						   0x46      //家的模式 L:1
#define TAG_TRACKER_DECLINATION					   0x47      //磁偏角 L:1
#define TAG_TRACKER_T_IP                           0x48      //Tarcker IP L:4
#define TAG_TRACKER_T_PORT                         0x49      //Tarcker Port L:2
#define TAG_TRACKER_S_IP                           0x4A      //SERVER IP L:4
#define TAG_TRACKER_S_PORT                         0x4B      //SERVER PORT :2
#define TAG_TRACKER_FLAG                           0x4C      //标志位 L:1
#define TAG_TRACKER_SEVRO_POWER                    0x4D      //舵机供电是否打开 L:1
//-----------------配置参数---------------------------------------------------
#define TAG_PARAM_PID_P							   0x70      //PID_P L:2
#define TAG_PARAM_PID_I							   0x71      //PID_I L:2
#define TAG_PARAM_PID_D							   0x72      //PID_D L:2
#define TAG_PARAM_TITL_0						   0x73      //俯仰零度 L:2
#define TAG_PARAM_TITL_90						   0x74      //俯仰90度 L:2
#define TAG_PARAM_PAN_0  						   0x75      //水平中立点 L:2
#define TAG_PARAM_OFFSET						   0x76      //罗盘偏移 L:2
#define TAG_PARAM_TRACKING_DISTANCE		           0x77      //开始跟踪距离 L:1
#define TAG_PARAM_MAX_PID_ERROR					   0x78      //跟踪偏移度数 L:1
#define TAG_PARAM_MIN_PAN_SPEED					   0x79      //最小舵机速度 L:1
#define TAG_PARAM_DECLINATION					   0x7A      //磁偏角 L:1
// 以下参数为iats_pro版本   
#define TAG_PARAM_SHOW_COORDINATE                  0x80      //是否在主界面显示坐标 L:1
#define TAG_PARAM_MONITOR_BATTERY_ENABLE           0x81      //是否监控电池电压 L:1
#define TAG_PARAM_MONITOR_BATTERY_VOLTAGE_SCALE    0x82      //电池电压分压系数 L:2
#define TAG_PARAM_MONITOR_BATTERY_MAX_VOLTAGE      0x83      //电池电压最大值 L:2
#define TAG_PARAM_MONITOR_BATTERY_MIN_VOLTAGE      0x84      //电池电压最小值 L:2
#define TAG_PARAM_MONITOR_BATTERY_CENTER_VOLTAGE   0x85      //电池电压中间值 L:2
#define TAG_PARAM_MONITOR_POWER_ENABLE             0x86      //监控舵机电源 L:1
#define TAG_PARAM_MONITOR_POWER_ON                 0x87      //舵机电源打开 L:1
#define TAG_PARAM_WIFI_SSID                        0x88      //WIFI SSID L:32
#define TAG_PARAM_WIFI_PWD                         0x89      //WIFI PWD L:32
#define TAG_PARAM_SERVO_COURSE                     0x8A      //正北位置舵机指向 L:2
#define TAG_PARAM_SERVO_PAN_MIN_PLUSEWIDTH         0x8B      //水平舵机最小PWM L:2
#define TAG_PARAM_SERVO_PAN_MAX_PLUSEWIDTH         0x8C      //水平舵机最大PWM L:2
#define TAG_PARAM_SERVO_PAN_MAX_DEGREE             0x8D      //水平舵机最大角度 L:2
#define TAG_PARAM_SERVO_PAN_MIN_DEGREE             0x8E      //水平舵机最小角度 L:2
#define TAG_PARAM_SERVO_PAN_DIRECTION              0x8F      //水平舵机方向 L:1
#define TAG_PARAM_SERVO_TILT_MIN_PLUSEWIDTH        0x90      //俯仰舵机最小PWM L:2
#define TAG_PARAM_SERVO_TILT_MAX_PLUSEWIDTH        0x91      //俯仰舵机最大PWM L:2
#define TAG_PARAM_SERVO_TILT_MAX_DEGREE            0x92      //俯仰舵机最大角度 L:2
#define TAG_PARAM_SERVO_TILT_MIN_DEGREE            0x93      //俯仰舵机最小角度 L:2
#define TAG_PARAM_SERVO_TILT_DIRECTION             0x94      //俯仰舵机方向 L:1
#define TAG_PARAM_SERVO_EASE_OUT_TYPE              0x95      //缓冲类型 L:1
#define TAG_PARAM_SERVO_EASE_MAX_STEPS             0x96      //缓冲最大步数 L:2
#define TAG_PARAM_SERVO_EASE_MIN_PULSEWIDTH        0x97      //缓冲最小PWM L:2
#define TAG_PARAM_SERVO_EASE_STEP_MS               0x98      //缓冲每步间隔（毫秒） L:2
#define TAG_PARAM_SERVO_EASE_MAX_MS                0x99      //缓冲最大时间（毫秒） L:2
#define TAG_PARAM_SERVO_EASE_MIN_MS                0x9A      //缓冲最小时间（毫秒） L:2
#define TAG_PARAM_SCREEN_BRIGHTNESS                0x9B      //屏幕亮度 L:1
#define TAG_PARAM_SCREEN_AUTO_OFF                  0x9C      //自动关屏 L:1
#define TAG_PARAM_BEEPER_ENABLE                    0x9D      //Beeper L:1
//-----------------控制参数---------------------------------------------------
#define TAG_CTR_MODE							   0xC0      //模式 L:1 0：手动模式 1：自动跟踪 2：调试模式
#define TAG_CTR_AUTO_POINT_TO_NORTH				   0xC1      //自动指北 L:1 0：不启用 1:启用
#define TAG_CTR_CALIBRATE				           0xC2      //校准 L:1 >0：开始校准
#define TAG_CTR_HEADING				               0xC3      //指向 L:2 0~359
#define TAG_CTR_TILT				               0xC4      //俯仰 L:1 0~90
#define TAG_CTR_REBOOT				               0xC5      //重启
   
//-----------------命令字---------------------------------------------------
#define CMD_HEARTBEAT							   0x00      //心跳
#define CMD_ACK                                    0x01      //应答结果
#define CMD_GET_AIRPLANE						   0x20      //上传飞机状态
#define CMD_GET_TRACKER							   0x21      //上传设备状态
#define CMD_GET_PARAM							   0x22      //上传参数
#define CMD_GET_HOME							   0x23      //设置家
#define CMD_SET_AIRPLANE						   0x50      //上传飞机状态
#define CMD_SET_TRACKER							   0x51      //上传设备状态
#define CMD_SET_PARAM							   0x52      //上传参数
#define CMD_SET_HOME							   0x53      //设置家
#define CMD_CONTROL 		    				   0xA0      //控制指令

#define TAG_STRING_MAX_SIZE 20
#define ATP_ASSERT_TYPE(id, typ) assert(telemetry_get_type(id) == typ)

#define ATP_FRAME_BUFFER_SIZE 256

// typedef struct tracker_s tracker_t;
typedef struct _Notifier notifier_t;

typedef enum
{
    IDLE = 0,
    STATE_LEAD,
    STATE_START,
    STATE_CMD,
    STATE_INDEX,
    STATE_LEN,
    STATE_DATA
} atp_frame_status_e;

typedef enum
{
    ATP_GPS_FIX_NONE = 0,
    ATP_GPS_FIX_2D,
    ATP_GPS_FIX_3D,
} atp_gps_fix_type_e;

typedef struct atp_tag_info_s
{
    uint8_t tag;
    telemetry_type_e type;
    const char *name;
    const char *(*format)(const telemetry_t *val, char *buf, size_t bufsize);
} atp_tag_info_t;

typedef struct atp_frame_s
{
    atp_frame_status_e atp_status;
    uint8_t atp_cmd;
    uint8_t atp_index;
    uint8_t atp_tag_len;
    uint8_t atp_crc;
    uint8_t buffer_index;
    uint8_t buffer[ATP_FRAME_BUFFER_SIZE];
} atp_frame_t;

typedef void (*pTr_atp_decode)(void *t, void *buffer, int offset, int len);
typedef void (*pTr_atp_send)(void *buffer, int len);
typedef void (*pTr_tag_value_changed)(void *t, uint8_t tag);

typedef struct atp_cmd_s
{
    uint8_t cmds[5];
    uint8_t is_cmd_doing;
    uint8_t param_index;
} atp_cmd_t;

typedef struct atp_ctr_s
{
    uint8_t ctrs[5];
    uint8_t data[32];
    uint8_t data_index;
} atp_ctr_t;

typedef struct atp_s
{
    pTr_atp_decode atp_decode;
    pTr_atp_send atp_send;
    pTr_tag_value_changed tag_value_changed;
    void *tracker;

    atp_frame_t *dec_frame;
    atp_frame_t *enc_frame;

    telemetry_t *plane_vals;
    telemetry_t *tracker_vals;
    telemetry_t *param_vals;
    // telemetry_t *iats_pro_param_vals;

    notifier_t *telemetry_val_notifier;

    atp_cmd_t *atp_cmd;
    atp_ctr_t *atp_ctr;
} atp_t;

void atp_init(atp_t *t);
uint8_t atp_get_tag_index(uint8_t tag);
uint8_t *atp_frame_encode(void *data);
telemetry_t *atp_get_telemetry_tag_val(uint8_t tag);
uint8_t atp_popup_cmd();
void atp_remove_ctr(uint8_t len);

#define ATP_SET_U8(tag, v, now) telemetry_set_u8(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_I8(tag, v, now) telemetry_set_i8(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_U16(tag, v, now) telemetry_set_u16(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_I16(tag, v, now) telemetry_set_i16(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_U32(tag, v, now) telemetry_set_u32(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_I32(tag, v, now) telemetry_set_i32(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_STR(tag, v, now) telemetry_set_str(atp_get_telemetry_tag_val(tag), v, now);
#define ATP_SET_BYTES(tag, v, now) telemetry_set_bytes(atp_get_telemetry_tag_val(tag), v, now);