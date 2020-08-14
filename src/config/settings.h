#include <stdbool.h>
#include <stdint.h>

#include "config/config.h"
#include "io/gpio.h"
#include "target/target.h"

#define SETTING_STRING_MAX_LENGTH 32
#define SETTING_STRING_BUFFER_SIZE (SETTING_STRING_MAX_LENGTH + 1)
#define SETTING_NAME_BUFFER_SIZE SETTING_STRING_BUFFER_SIZE
#define SETTING_STATIC_COUNT 1
#define SETTING_TEMP_COUNT 15

#define SETTING_TRACKER_FOLDER_COUNT 6
#define SETTING_ESTIMATE_FOLDER_COUNT 2
#define SETTING_ADVANCED_POS_FOLDER_COUNT 2
#define SETTING_HOME_FOLDER_COUNT 8
#if defined(USE_WIFI)
#define SETTING_WIFI_FOLDER_COUNT 6
#else
#define SETTING_WIFI_FOLDER_COUNT 0
#endif
#define SETTING_SERVO_FOLDER_COUNT 2
#define SETTING_SERVO_PAN_FOLDER_COUNT 6
#define SETTING_SERVO_TILT_FOLDER_COUNT 6
#define SETTING_EASE_FOLDER_COUNT 7

#define SETTING_PORT_FOLDER_COUNT 3
#define SETTING_PORT_UART1_FOLDER_COUNT 4
#define SETTING_PORT_UART2_FOLDER_COUNT 4
// #define SETTING_PORT_UART1_FOLDER_COUNT 6
// #define SETTING_PORT_UART2_FOLDER_COUNT 6

#if defined(USE_MONITORING)
#define SETTING_MONITOR_FOLDER_COUNT 1
#else
#define SETTING_MONITOR_FOLDER_COUNT 0
#endif

#if defined(USE_BATTERY_MONITORING)
#define SETTING_BATTERY_FOLDER_COUNT 7
#else
#define SETTING_BATTERY_FOLDER_COUNT 0
#endif

#if defined(USE_POWER_MONITORING)
#define SETTING_POWER_FOLDER_COUNT 3
#else
#define SETTING_POWER_FOLDER_COUNT 0
#endif

#if defined(USE_SCREEN)
#define SETTING_SCREEN_FOLDER_COUNT 3
#else
#define SETTING_SCREEN_FOLDER_COUNT 0
#endif

#if defined(USE_BEEPER)
#define SETTING_BEEPER_FOLDER_COUNT 2
#else
#define SETTING_BEEPER_FOLDER_COUNT 0
#endif

#if defined(USE_IMU)
#define SETTING_IMU_FOLDER_COUNT 4
#define SETTING_IMU_CALIBRATION_FOLDER_COUNT 3
#else
#define SETTING_IMU_FOLDER_COUNT 0
#define SETTING_IMU_CALIBRATION_FOLDER_COUNT 0
#endif

#define SETTING_DIAGNOSTICS_FOLDER_COUNT 2
#define SETTING_DEVELOPER_FOLDER_COUNT 2

#define SETTING_COUNT (SETTING_STATIC_COUNT + SETTING_TRACKER_FOLDER_COUNT + SETTING_ESTIMATE_FOLDER_COUNT + SETTING_ADVANCED_POS_FOLDER_COUNT + SETTING_HOME_FOLDER_COUNT + SETTING_MONITOR_FOLDER_COUNT + SETTING_BATTERY_FOLDER_COUNT + SETTING_POWER_FOLDER_COUNT + SETTING_WIFI_FOLDER_COUNT + SETTING_PORT_FOLDER_COUNT + SETTING_PORT_UART1_FOLDER_COUNT + SETTING_PORT_UART2_FOLDER_COUNT + SETTING_SERVO_FOLDER_COUNT + SETTING_SERVO_PAN_FOLDER_COUNT + SETTING_SERVO_TILT_FOLDER_COUNT + SETTING_EASE_FOLDER_COUNT + SETTING_SCREEN_FOLDER_COUNT + SETTING_BEEPER_FOLDER_COUNT + SETTING_IMU_FOLDER_COUNT + SETTING_IMU_CALIBRATION_FOLDER_COUNT + SETTING_DIAGNOSTICS_FOLDER_COUNT + SETTING_DEVELOPER_FOLDER_COUNT)

#define SETTING_KEY_TRACKER "t"
#define SETTING_KEY_TRACKER_PREFIX SETTING_KEY_TRACKER "."
#define SETTING_KEY_TRACKER_SHOW_COORDINATE SETTING_KEY_TRACKER_PREFIX "show c"
#define SETTING_KEY_TRACKER_REAL_ALT SETTING_KEY_TRACKER_PREFIX "real alt"

#define SETTING_KEY_TRACKER_ESTIMATE SETTING_KEY_TRACKER_PREFIX "e"
#define SETTING_KEY_TRACKER_ESTIMATE_PREFIX SETTING_KEY_TRACKER_ESTIMATE "."
#define SETTING_KEY_TRACKER_ESTIMATE_ENABLE SETTING_KEY_TRACKER_ESTIMATE_PREFIX "Enable"
#define SETTING_KEY_TRACKER_ESTIMATE_SECOND SETTING_KEY_TRACKER_ESTIMATE_PREFIX "E.Sec"

#define SETTING_KEY_TRACKER_ADVANCED_POS SETTING_KEY_TRACKER_PREFIX "e"
#define SETTING_KEY_TRACKER_ADVANCED_POS_PREFIX SETTING_KEY_TRACKER_ADVANCED_POS "."
#define SETTING_KEY_TRACKER_ADVANCED_POS_ENABLE SETTING_KEY_TRACKER_ADVANCED_POS "Enable"
#define SETTING_KEY_TRACKER_ADVANCED_POS_SECOND SETTING_KEY_TRACKER_ADVANCED_POS "A.Sec"

#define SETTING_KEY_TRACKER_HOME "h"
#define SETTING_KEY_TRACKER_HOME_PREFIX SETTING_KEY_TRACKER_HOME "."
#define SETTING_KEY_TRACKER_HOME_SET SETTING_KEY_TRACKER_HOME_PREFIX "set"
#define SETTING_KEY_TRACKER_HOME_RECOVER SETTING_KEY_TRACKER_HOME_PREFIX "rec"
#define SETTING_KEY_TRACKER_HOME_CLEAR SETTING_KEY_TRACKER_HOME_PREFIX "clear"
#define SETTING_KEY_TRACKER_HOME_SOURCE SETTING_KEY_TRACKER_HOME_PREFIX "src"
#define SETTING_KEY_TRACKER_HOME_REAL_TIME SETTING_KEY_TRACKER_HOME_PREFIX "rt"
#define SETTING_KEY_TRACKER_HOME_LAT SETTING_KEY_TRACKER_HOME_PREFIX "lat"
#define SETTING_KEY_TRACKER_HOME_LON SETTING_KEY_TRACKER_HOME_PREFIX "lon"
#define SETTING_KEY_TRACKER_HOME_ALT SETTING_KEY_TRACKER_HOME_PREFIX "alt"

#if defined(USE_MONITORING)
#define SETTING_KEY_TRACKER_MONITOR SETTING_KEY_TRACKER_PREFIX "m"
#define SETTING_KEY_TRACKER_MONITOR_PREFIX SETTING_KEY_TRACKER_MONITOR "."

#if defined(USE_BATTERY_MONITORING)
#define SETTING_KEY_TRACKER_MONITOR_BATTERY SETTING_KEY_TRACKER_MONITOR_PREFIX "b"
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX SETTING_KEY_TRACKER_MONITOR_BATTERY "."
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_ENABLE SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX "Enable"
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_VOLTAGE SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX "Voltage"
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_VOLTAGE_SCALE SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX "V. Scale"
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_MAX_VOLTAGE SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX "Max V."
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_MIN_VOLTAGE SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX "Min V."
#define SETTING_KEY_TRACKER_MONITOR_BATTERY_CENTER_VOLTAGE SETTING_KEY_TRACKER_MONITOR_BATTERY_PREFIX "Center V."
#endif

#if defined(USE_POWER_MONITORING)
#define SETTING_KEY_TRACKER_MONITOR_POWER SETTING_KEY_TRACKER_MONITOR_PREFIX "p"
#define SETTING_KEY_TRACKER_MONITOR_POWER_PREFIX SETTING_KEY_TRACKER_MONITOR_POWER "."
#define SETTING_KEY_TRACKER_MONITOR_POWER_ENABLE SETTING_KEY_TRACKER_MONITOR_POWER_PREFIX "Enable"
#define SETTING_KEY_TRACKER_MONITOR_POWER_TURN SETTING_KEY_TRACKER_MONITOR_POWER_PREFIX "Turn"
#endif
#endif

#if defined(USE_WIFI)
#define SETTING_KEY_WIFI "wifi"
#define SETTING_KEY_WIFI_PREFIX SETTING_KEY_WIFI "."
#define SETTING_KEY_WIFI_ENABLE SETTING_KEY_WIFI_PREFIX "Enable"
#define SETTING_KEY_WIFI_SSID SETTING_KEY_WIFI_PREFIX "ssid"
#define SETTING_KEY_WIFI_PWD SETTING_KEY_WIFI_PREFIX "pwd"
#define SETTING_KEY_WIFI_IP SETTING_KEY_WIFI_PREFIX "ip"
#define SETTING_KEY_WIFI_SMART_CONFIG SETTING_KEY_WIFI_PREFIX "sc"
#endif

#define SETTING_KEY_PORT "port"
#define SETTING_KEY_PORT_PREFIX SETTING_KEY_PORT "."

#define SETTING_KEY_PORT_UART1 SETTING_KEY_PORT_PREFIX "u1"
#define SETTING_KEY_PORT_UART1_PREFIX SETTING_KEY_PORT_UART1 "."
#define SETTING_KEY_PORT_UART1_ENABLE SETTING_KEY_PORT_UART1_PREFIX "Enable"
#define SETTING_KEY_PORT_UART1_TYPE SETTING_KEY_PORT_UART1_PREFIX "t"
#define SETTING_KEY_PORT_UART1_PROTOCOL SETTING_KEY_PORT_UART1_PREFIX "p"
#define SETTING_KEY_PORT_UART1_PIN_TX SETTING_KEY_PORT_UART1_PREFIX "tx"
#define SETTING_KEY_PORT_UART1_PIN_RX SETTING_KEY_PORT_UART1_PREFIX "rx"
#define SETTING_KEY_PORT_UART1_BAUDRATE SETTING_KEY_PORT_UART1_PREFIX "rate"

#define SETTING_KEY_PORT_UART2 SETTING_KEY_PORT_PREFIX "u2"
#define SETTING_KEY_PORT_UART2_PREFIX SETTING_KEY_PORT_UART2 "."
#define SETTING_KEY_PORT_UART2_ENABLE SETTING_KEY_PORT_UART2_PREFIX "Enable"
#define SETTING_KEY_PORT_UART2_TYPE SETTING_KEY_PORT_UART2_PREFIX "t"
#define SETTING_KEY_PORT_UART2_PROTOCOL SETTING_KEY_PORT_UART2_PREFIX "p"
#define SETTING_KEY_PORT_UART2_PIN_TX SETTING_KEY_PORT_UART2_PREFIX "tx"
#define SETTING_KEY_PORT_UART2_PIN_RX SETTING_KEY_PORT_UART2_PREFIX "rx"
#define SETTING_KEY_PORT_UART2_BAUDRATE SETTING_KEY_PORT_UART2_PREFIX "rate"

#define SETTING_KEY_SERVO "svo"
#define SETTING_KEY_SERVO_PREFIX SETTING_KEY_SERVO "."
#define SETTING_KEY_SERVO_COURSE SETTING_KEY_SERVO_PREFIX "course"

#define SETTING_KEY_SERVO_PAN SETTING_KEY_SERVO_PREFIX "p"
#define SETTING_KEY_SERVO_PAN_PREFIX SETTING_KEY_SERVO_PAN "."
#define SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH SETTING_KEY_SERVO_PAN_PREFIX "min pwm"
#define SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH SETTING_KEY_SERVO_PAN_PREFIX "max pwm"
#define SETTING_KEY_SERVO_PAN_MAX_DEGREE SETTING_KEY_SERVO_PAN_PREFIX "max deg"
#define SETTING_KEY_SERVO_PAN_MIN_DEGREE SETTING_KEY_SERVO_PAN_PREFIX "min deg"
#define SETTING_KEY_SERVO_PAN_DIRECTION SETTING_KEY_SERVO_PAN_PREFIX "d."

#define SETTING_KEY_SERVO_TILT SETTING_KEY_SERVO_PREFIX "t"
#define SETTING_KEY_SERVO_TILT_PREFIX SETTING_KEY_SERVO_TILT "."
#define SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH SETTING_KEY_SERVO_TILT_PREFIX "max pwm"
#define SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH SETTING_KEY_SERVO_TILT_PREFIX "min pwm"
#define SETTING_KEY_SERVO_TILT_MAX_DEGREE SETTING_KEY_SERVO_TILT_PREFIX "max deg"
#define SETTING_KEY_SERVO_TILT_MIN_DEGREE SETTING_KEY_SERVO_TILT_PREFIX "min deg"
#define SETTING_KEY_SERVO_TILT_DIRECTION SETTING_KEY_SERVO_TILT_PREFIX "d."

#define SETTING_KEY_SERVO_EASE SETTING_KEY_SERVO_PREFIX "e"
#define SETTING_KEY_SERVO_EASE_PREFIX SETTING_KEY_SERVO_EASE "."
#define SETTING_KEY_SERVO_EASE_OUT_TYPE SETTING_KEY_SERVO_EASE_PREFIX "type"
#define SETTING_KEY_SERVO_EASE_MAX_STEPS SETTING_KEY_SERVO_EASE_PREFIX "max steps"
#define SETTING_KEY_SERVO_EASE_MIN_PULSEWIDTH SETTING_KEY_SERVO_EASE_PREFIX "min pwm"
#define SETTING_KEY_SERVO_EASE_STEP_MS SETTING_KEY_SERVO_EASE_PREFIX "step ms"
#define SETTING_KEY_SERVO_EASE_MAX_MS SETTING_KEY_SERVO_EASE_PREFIX "max ms"
#define SETTING_KEY_SERVO_EASE_MIN_MS SETTING_KEY_SERVO_EASE_PREFIX "min ms"

#if defined(USE_SCREEN)
#define SETTING_KEY_SCREEN "scr" // Using screen here makes esp32 NVS return "key-too-long"
#define SETTING_KEY_SCREEN_PREFIX SETTING_KEY_SCREEN "."
#define SETTING_KEY_SCREEN_BRIGHTNESS SETTING_KEY_SCREEN_PREFIX "brightness"
#define SETTING_KEY_SCREEN_AUTO_OFF SETTING_KEY_SCREEN_PREFIX "auto_off"
#endif

#if defined(USE_BEEPER)
#define SETTING_KEY_BEEPER "bep"
#define SETTING_KEY_BEEPER_PREFIX SETTING_KEY_BEEPER "."
#define SETTING_KEY_BEEPER_ENABLE SETTING_KEY_BEEPER_PREFIX "Enable"
#endif

#if defined(USE_IMU)
#define SETTING_KEY_IMU "imu"
#define SETTING_KEY_IMU_PREFIX SETTING_KEY_IMU "."
#define SETTING_KEY_IMU_ENABLE SETTING_KEY_IMU_PREFIX "Enable"
#define SETTING_KEY_IMU_INFO SETTING_KEY_IMU_PREFIX "info"

#define SETTING_KEY_IMU_CALIBRATION SETTING_KEY_IMU_PREFIX "c"
#define SETTING_KEY_IMU_CALIBRATION_PREFIX SETTING_KEY_IMU_PREFIX "."
#define SETTING_KEY_IMU_CALIBRATION_ACC SETTING_KEY_IMU_CALIBRATION_PREFIX "acc"
#define SETTING_KEY_IMU_CALIBRATION_GYRO SETTING_KEY_IMU_CALIBRATION_PREFIX "gyro"
#define SETTING_KEY_IMU_CALIBRATION_MAG SETTING_KEY_IMU_CALIBRATION_PREFIX "mag"
#endif

#define SETTING_KEY_DIAGNOSTICS "diag"
#define SETTING_KEY_DIAGNOSTICS_PREFIX SETTING_KEY_DIAGNOSTICS "."
#define SETTING_KEY_DIAGNOSTICS_DEBUG_INFO SETTING_KEY_DIAGNOSTICS_PREFIX "dbg-i"

#define SETTING_KEY_DEVELOPER "dev"
#define SETTING_KEY_DEVELOPER_PREFIX SETTING_KEY_DEVELOPER "."
#define SETTING_KEY_DEVELOPER_REBOOT SETTING_KEY_DEVELOPER_PREFIX "rbt"

#define SETTING_IS(setting, k) STR_EQUAL(setting->key, k)

typedef enum
{
    FOLDER_ID_ROOT = 1,
    FOLDER_ID_TRACKER,
    FOLDER_ID_ESTIMATE,
    FOLDER_ID_ADVANCED_POS,
    FOLDER_ID_HOME,
    FOLDER_ID_MONITOR,
    FOLDER_ID_BATTERY,
    FOLDER_ID_POWER,
    FOLDER_ID_WIFI,
    FOLDER_ID_PORT,
    FOLDER_ID_UART1,
    FOLDER_ID_UART2,
    FOLDER_ID_SERVO,
    FOLDER_ID_PAN,
    FOLDER_ID_TILT,
    FOLDER_ID_EASE,
    FOLDER_ID_SCREEN,
    FOLDER_ID_BEEPER,
    FOLDER_ID_IMU,
    FOLDER_ID_CALIBRATION,
    FOLDER_ID_DIAGNOSTICS,
    FOLDER_ID_DEVELOPER,
} folder_id_e;

typedef enum
{
    SETTING_CMD_FLAG_WARNING,
    SETTING_CMD_FLAG_CONFIRM,
} setting_cmd_flag_e;

typedef enum
{
    SETTING_CMD_STATUS_NONE = 0,
    SETTING_CMD_STATUS_CHANGE = 1,
    SETTING_CMD_STATUS_SHOW_WARNING = 2,
    SETTING_CMD_STATUS_ASK_CONFIRM = 3,
    SETTING_CMD_STATUS_COMMIT = 4,
    SETTING_CMD_STATUS_DISCARD = 5,
    SETTING_CMD_STATUS_PING = 0xFF,
} setting_cmd_status_e;

typedef enum
{
    SETTING_TYPE_U8 = 0,
    SETTING_TYPE_I8,
    SETTING_TYPE_U16,
    SETTING_TYPE_I16,
    SETTING_TYPE_U32,
    SETTING_TYPE_I32,
    SETTING_TYPE_STRING = 6,
    SETTING_TYPE_FOLDER = 7,
} setting_type_e;

typedef enum
{
    SETTING_FLAG_NAME_MAP = 1 << 0,
    SETTING_FLAG_EPHEMERAL = 1 << 1,
    SETTING_FLAG_READONLY = 1 << 2,
    SETTING_FLAG_CMD = 1 << 3,
    SETTING_FLAG_DYNAMIC = 1 << 4,
    SETTING_FLAG_VALUE = 1 << 5,
    SETTING_FLAG_FLOAT = 1 << 6,
    SETTING_FLAG_IGNORE_CHANGE = 1 << 7,
    SETTING_FLAG_COORDINATE = 1 << 8,
} setting_flag_e;

typedef union {
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    int16_t i16;
    uint32_t u32;
    int32_t i32;
} setting_value_t;

typedef struct setting_s
{
    const char *key;
    const char *name;
    const setting_type_e type;
    const setting_flag_e flags;
    const char **val_names;
    const char *unit;
    const folder_id_e folder;
    const setting_value_t min;
    const setting_value_t max;
    const setting_value_t def_val;
    const uint8_t tmp_index;
    const float float_divisor;
    const void *data;
} setting_t;

typedef enum
{
    SETTINGS_VIEW_CRSF_INPUT,
    SETTINGS_VIEW_MENU,   // On-device menu
    SETTINGS_VIEW_REMOTE, // Remote settings (other device)
} settings_view_e;

typedef struct settings_view_s
{
    uint8_t indexes[SETTING_COUNT];
    int count;
} settings_view_t;

void settings_init(void);

typedef void (*setting_changed_f)(const setting_t *setting, void *user_data);

void settings_add_listener(setting_changed_f callback, void *user_data);
void settings_remove_listener(setting_changed_f callback, void *user_data);

int settings_get_count(void);

const setting_t *settings_get_setting_at(int idx);
const setting_t *settings_get_key(const char *key);
uint8_t settings_get_key_u8(const char *key);
int8_t settings_get_key_i8(const char *key);
uint16_t settings_get_key_u16(const char *key);
int16_t settings_get_key_i16(const char *key);
uint32_t settings_get_key_u32(const char *key);
int32_t settings_get_key_i32(const char *key);
hal_gpio_t settings_get_key_gpio(const char *key);
bool settings_get_key_bool(const char *key);
const char *settings_get_key_string(const char *key);
const setting_t *settings_get_key_idx(const char *key, int *idx);
const setting_t *settings_get_folder(folder_id_e folder);
bool settings_is_folder_visible(settings_view_e view_id, folder_id_e folder);
bool setting_is_visible(settings_view_e view_id, const char *key);

folder_id_e setting_get_folder_id(const setting_t *setting);
folder_id_e setting_get_parent_folder_id(const setting_t *setting);
int32_t setting_get_min(const setting_t *setting);
int32_t setting_get_max(const setting_t *setting);
int32_t setting_get_default(const setting_t *setting);
uint8_t setting_get_u8(const setting_t *setting);
int8_t setting_get_i8(const setting_t *setting);
uint16_t setting_get_u16(const setting_t *setting);
int16_t setting_get_i16(const setting_t *setting);
uint32_t setting_get_u32(const setting_t *setting);
int32_t setting_get_i32(const setting_t *setting);
void setting_set_u8(const setting_t *setting, uint8_t v);
void setting_set_i8(const setting_t *setting, int8_t v);
void setting_set_u16(const setting_t *setting, uint16_t v);
void setting_set_i16(const setting_t *setting, int16_t v);
void setting_set_u32(const setting_t *setting, uint32_t v);
void setting_set_i32(const setting_t *setting, int32_t v);
hal_gpio_t setting_get_gpio(const setting_t *setting);
bool setting_get_bool(const setting_t *setting);
void setting_set_bool(const setting_t *setting, bool v);
const char *setting_get_string(const setting_t *setting);
void setting_set_string(const setting_t *setting, const char *s);

const char *setting_map_name(const setting_t *setting, uint8_t val);
void setting_format_name(char *buf, size_t size, const setting_t *setting);
void setting_format(char *buf, size_t size, const setting_t *setting);
void setting_format_value(char *buf, size_t size, const setting_t *setting);

// These functions are only valid for settings with the SETTING_FLAG_CMD flag
setting_cmd_flag_e setting_cmd_get_flags(const setting_t *setting);
bool setting_cmd_exec(const setting_t *setting);

int setting_rx_channel_output_get_pos(const setting_t *setting);
int setting_receiver_get_rx_num(const setting_t *setting);

void setting_increment(const setting_t *setting);
void setting_decrement(const setting_t *setting);

bool settings_view_get(settings_view_t *view, settings_view_e view_id, folder_id_e folder);
bool settings_view_get_folder_view(settings_view_t *view, settings_view_e view_id, folder_id_e folder, bool recursive);
const setting_t *settings_view_get_setting_at(settings_view_t *view, int idx);
int settings_view_get_parent_index(settings_view_t *view, const setting_t *setting);

bool setting_is_has_tmp(const setting_t *setting);
bool setting_get_value_is_tmp(const setting_t *setting);
void setting_set_value_is_tmp(const setting_t *setting, bool is_tmp);
uint16_t setting_get_tmp_u16(const setting_t *setting);
void setting_set_tmp_u16(const setting_t *setting, uint16_t v);