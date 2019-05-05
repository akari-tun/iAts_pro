#include <stdint.h>

#include "util/macros.h"
#include "util/time.h"
#include "util/data_state.h"
#include "tracker/telemetry.h"

#define MAX_TAG_COUNT							10        //每帧数据最大TAG数
#define MAX_CMD_COUNT							5         //最大缓存等待发送的指令数

#define TP_PACKET_LEAD							0x24      //引导码 $
#define TP_PACKET_START							0x54      //协议头 T

#define TAG_COUNT								37        //TAG数量，定义了新的TAG需要增加这个值
#define TAG_BASE_COUNT                          3         //基础Tag数量
#define TAG_PLANE_COUNT                         10        //Plane tags count
#define TAG_HOME_COUNT                          8         //Home tags count
#define TAG_PARAM_COUNT                         11        //Parameter tags count

#define TAG_PLANE_MASK                          0x10      //Plane tags mask
#define TAG_HOME_MASK                           0x40      //Home tags mask
#define TAG_PARAM_MASK                          0x70      //Parameter tags mask

//-----------------基础协议---------------------------------------------------
#define TAG_BASE_ACK							0x00      //应答结果 L:1 V:0 失败 非0成功，成功应答命令的INDEX
#define TAG_BASE_HEARTBEAT						0x01	  //设备心跳 L:1 V:0空闲 1正在跟踪 2调试模式 3手动模式
#define TAG_BASE_QUERY							0x02	  //请求数据 L:1 V:CMD 请求的指令的CMD值
//-----------------飞机数据---------------------------------------------------
#define TAG_PLANE_LONGITUDE						0x10      //飞机经度 L:4 
#define TAG_PLANE_LATITUDE						0x11      //飞机纬度 L:4
#define TAG_PLANE_ALTITUDE						0x12      //飞机高度 L:4
#define TAG_PLANE_SPEED							0x13      //飞机速度 L:2
#define TAG_PLANE_DISTANCE						0x14      //飞机距离 L:4
#define TAG_PLANE_STAR							0x15      //定位星数 L:1
#define TAG_PLANE_FIX							0x16      //定位类型 L:1
#define TAG_PLANE_PITCH							0x17      //俯仰角度 L:2
#define TAG_PLANE_ROLL							0x18      //横滚角度 L:2
#define TAG_PLANE_HEADING						0x19      //飞机方向 L:2
//-----------------家的数据---------------------------------------------------
#define TAG_HOME_LONGITUDE						0x40      //家的经度 L:4
#define TAG_HOME_LATITUDE						0x41      //家的纬度 L:4
#define TAG_HOME_ALTITUDE						0x42      //家的高度 L:4
#define TAG_HOME_HEADING						0x43      //家的朝向 L:2
#define TAG_HOME_PITCH  						0x44      //家的俯仰 L:1
#define TAG_HOME_VOLTAGE						0x45      //家的电压 L:2
#define TAG_HOME_MODE							0x46      //家的模式 L:1
#define TAG_HOME_DECLINATION					0x47      //磁偏角 L:1
//-----------------配置参数---------------------------------------------------
#define TAG_PARAM_PID_P							0x70      //PID_P L:2
#define TAG_PARAM_PID_I							0x71      //PID_I L:2
#define TAG_PARAM_PID_D							0x72      //PID_D L:2
#define TAG_PARAM_TITL_0						0x73      //俯仰零度 L:2
#define TAG_PARAM_TITL_90						0x74      //俯仰90度 L:2
#define TAG_PARAM_PAN_0  						0x75      //水平中立点 L:2
#define TAG_PARAM_OFFSET						0x76      //罗盘偏移 L:2
#define TAG_PARAM_TRACKING_DISTANCE		        0x77      //开始跟踪距离 L:1
#define TAG_PARAM_MAX_PID_ERROR					0x78      //跟踪偏移度数 L:1
#define TAG_PARAM_MIN_PAN_SPEED					0x79      //最小舵机速度 L:1
#define TAG_PARAM_DECLINATION					0x7A      //磁偏角 L:1
//-----------------控制参数---------------------------------------------------
#define TAG_CTR_MODE							0xA0      //模式 L:1 0：手动模式 1：自动跟踪 2：调试模式
#define TAG_CTR_AUTO_POINT_TO_NORTH				0xA1      //自动指北 L:1 0：不启用 1:启用
#define TAG_CTR_CALIBRATE				        0xA2      //校准 L:1 >0：开始校准
#define TAG_CTR_HEADING				            0xA3      //指向 L:2 0~359
#define TAG_CTR_TILT				            0xA4      //俯仰 L:1 0~90

//-----------------命令字---------------------------------------------------
#define CMD_HEARTBEAT							0x00      //心跳
#define CMD_ACK                                 0x01      //应答结果
#define CMD_GET_AIRPLANE						0x20      //上传飞机状态
#define CMD_GET_TRACKER							0x21      //上传设备状态
#define CMD_GET_PARAM							0x22      //上传参数
#define CMD_GET_HOME							0x23      //设置家
#define CMD_SET_AIRPLANE						0x50      //上传飞机状态
#define CMD_SET_TRACKER							0x51      //上传设备状态
#define CMD_SET_PARAM							0x52      //上传参数
#define CMD_SET_HOME							0x53      //设置家
#define CMD_CONTROL 		    				0xA0      //控制指令

#define TAG_STRING_MAX_SIZE 20
#define ATP_ASSERT_TYPE(id, typ) assert(telemetry_get_type(id) == typ)

typedef struct tracker_s tracker_t;

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
    uint8_t atp_tag_index;
    uint8_t *atp_tags;
    uint8_t atp_crc;
} atp_frame_t;

void atp_init(tracker_t *t);
uint8_t atp_get_tag_index(uint8_t tag);
telemetry_t *atp_get_tag_val(uint8_t tag);

#define ATP_SET_U8(tag, v, now) telemetry_set_u8(atp_get_tag_val(tag), v, now);
#define ATP_SET_I8(tag, v, now) telemetry_set_i8(atp_get_tag_val(tag), v, now);
#define ATP_SET_U16(tag, v, now) telemetry_set_u16(atp_get_tag_val(tag), v, now);
#define ATP_SET_I16(tag, v, now) telemetry_set_i16(atp_get_tag_val(tag), v, now);
#define ATP_SET_U32(tag, v, now) telemetry_set_u32(atp_get_tag_val(tag), v, now);
#define ATP_SET_I32(tag, v, now) telemetry_set_i32(atp_get_tag_val(tag), v, now);
#define ATP_SET_STR(tag, v, now) telemetry_set_str(atp_get_tag_val(tag), v, now);
#define ATP_SET_BYTES(tag, v, now) telemetry_set_bytes(atp_get_tag_val(tag), v, now);