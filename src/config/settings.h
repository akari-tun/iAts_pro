#include <stdbool.h>
#include <stdint.h>

#include "target/target.h"
#include "config/config.h"

#define SETTING_STRING_MAX_LENGTH 32
#define SETTING_STRING_BUFFER_SIZE (SETTING_STRING_MAX_LENGTH + 1)
#define SETTING_NAME_BUFFER_SIZE SETTING_STRING_BUFFER_SIZE
#define SETTING_STATIC_COUNT 1

#define SETTING_WIFI_FOLDER_COUNT 4
#define SETTING_SERVO_FOLDER_COUNT 8

#define SETTING_COUNT (SETTING_STATIC_COUNT + SETTING_WIFI_FOLDER_COUNT + SETTING_SERVO_FOLDER_COUNT)

#define SETTING_KEY_WIFI "wifi"
#define SETTING_KEY_WIFI_PREFIX SETTING_KEY_WIFI "."
#define SETTING_KEY_WIFI_SSID SETTING_KEY_WIFI_PREFIX "ssid"
#define SETTING_KEY_WIFI_PWD SETTING_KEY_WIFI_PREFIX "pwd"
#define SETTING_KEY_WIFI_SMART_CONFIG SETTING_KEY_WIFI_PREFIX "sc"
#define SETTING_KEY_SERVO "servo"
#define SETTING_KEY_SERVO_PREFIX SETTING_KEY_SERVO "."
#define SETTING_KEY_SERVO_COURSE SETTING_KEY_SERVO_PREFIX "course"
#define SETTING_KEY_SERVO_MAX_PLUSEWIDTH SETTING_KEY_SERVO_PREFIX "max pwm"
#define SETTING_KEY_SERVO_MIN_PLUSEWIDTH SETTING_KEY_SERVO_PREFIX "min pwm"
#define SETTING_KEY_SERVO_MAX_DEGREE SETTING_KEY_SERVO_PREFIX "max deg"
#define SETTING_KEY_SERVO_MIN_DEGREE SETTING_KEY_SERVO_PREFIX "min deg"
#define SETTING_KEY_SERVO_PAN_ZERO_DEGREE_PLUSEWIDTH SETTING_KEY_SERVO_PREFIX "pan zero"
#define SETTING_KEY_SERVO_TILT_ZERO_DEGREE_PLUSEWIDTH SETTING_KEY_SERVO_PREFIX "tilt zero"


#define SETTING_IS(setting, k) STR_EQUAL(setting->key, k)

typedef enum
{
    FOLDER_ID_ROOT = 1,
    FOLDER_ID_WIFI,
    FOLDER_ID_SERVO,
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