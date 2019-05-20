#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hal/log.h>

#include "platform/storage.h"
#include "platform/system.h"

#include "ui/screen.h"
#include "ui/ui.h"

#include "util/macros.h"
#include "util/version.h"

#include "settings.h"

static const char *TAG = "Settings";

#define MAX_SETTING_KEY_LENGTH 15

typedef enum
{
    SETTING_VISIBILITY_SHOW,
    SETTING_VISIBILITY_HIDE,
    SETTING_VISIBILITY_MOVE_CONTENTS_TO_PARENT,
} setting_visibility_e;

typedef enum
{
    SETTING_DYNAMIC_FORMAT_NAME,
    SETTING_DYNAMIC_FORMAT_VALUE,
} setting_dynamic_format_e;

// clang-format off
#define NO_VALUE {0}
#define BOOL(v) {.u8 = (v ? 1 : 0)}
#define U8(v) {.u8 = v}
#define I8(v) {.i8 = v}
#define U16(v) {.u16 = v}
#define I16(v) {.i16 = v}
#define U32(v) {.u32 = v}
#define I32(v) {.i32 = v}
// clang-format on

static char settings_string_storage[3][SETTING_STRING_BUFFER_SIZE];

#define SETTING_SHOW_IF(c) ((c) ? SETTING_VISIBILITY_SHOW : SETTING_VISIBILITY_HIDE)
#define SETTING_SHOW_IF_SCREEN(view_id) SETTING_SHOW_IF(view_id == SETTINGS_VIEW_MENU && system_has_flag(SYSTEM_FLAG_SCREEN))

static const char *off_on_table[] = {"Off", "On"};

#define FOLDER(k, n, id, p, fn) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_FOLDER, .flags = SETTING_FLAG_READONLY | SETTING_FLAG_EPHEMERAL, .folder = p, .def_val = U8(id), .data = fn, .tmp_index = 0 }
#define STRING_SETTING(k, n, p) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_STRING, .folder = p, .tmp_index = 0 }
#define FLAGS_STRING_SETTING(k, n, f, p, d) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_STRING, .flags = f, .folder = p, .data = d, .tmp_index = 0 }
#define RO_STRING_SETTING(k, n, p, v) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_STRING, .flags = SETTING_FLAG_READONLY, .folder = p, .data = v, .tmp_index = 0 }
#define U8_SETTING(k, n, f, p, mi, ma, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_U8, .flags = f, .folder = p, .min = U8(mi), .max = U8(ma), .def_val = U8(def), .tmp_index = 0 }
#define I8_SETTING(k, n, f, p, mi, ma, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_I8, .flags = f, .folder = p, .min = I8(mi), .max = I8(ma), .def_val = I8(def), .tmp_index = 0 }
#define U16_SETTING(k, n, f, p, mi, ma, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_U16, .flags = f, .folder = p, .min = U16(mi), .max = U16(ma), .def_val = U16(def), .tmp_index = 0 }
#define U16_HAS_TMP_SETTING(k, n, f, p, mi, ma, def, i) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_U16, .flags = f, .folder = p, .min = U16(mi), .max = U16(ma), .def_val = U16(def), .tmp_index = i }
#define I16_SETTING(k, n, f, p, mi, ma, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_I16, .flags = f, .folder = p, .min = I16(mi), .max = I16(ma), .def_val = I16(def), .tmp_index = 0 }
#define U32_SETTING(k, n, f, p, mi, ma, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_U32, .flags = f, .folder = p, .min = U32(mi), .max = U32(ma), .def_val = U32(def), .tmp_index = 0 }
#define I32_SETTING(k, n, f, p, mi, ma, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_I32, .flags = f, .folder = p, .min = I32(mi), .max = I32(ma), .def_val = I32(def), .tmp_index = 0 }
#define U8_MAP_SETTING_UNIT(k, n, f, p, m, u, def) \
    (setting_t) { .key = k, .name = n, .type = SETTING_TYPE_U8, .flags = f | SETTING_FLAG_NAME_MAP, .val_names = m, .unit = u, .folder = p, .min = U8(0), .max = U8(ARRAY_COUNT(m) - 1), .def_val = U8(def), .tmp_index = 0 }
#define U8_MAP_SETTING(k, n, f, p, m, def) U8_MAP_SETTING_UNIT(k, n, f, p, m, NULL, def)
#define BOOL_SETTING(k, n, f, p, def) U8_MAP_SETTING(k, n, f, p, off_on_table, def ? 1 : 0)
#define BOOL_YN_SETTING(k, n, f, p, def) U8_MAP_SETTING(k, n, f, p, no_yes_table, def ? 1 : 0)

#define CMD_SETTING(k, n, p, f, c_fl) U8_SETTING(k, n, f | SETTING_FLAG_EPHEMERAL | SETTING_FLAG_CMD, p, 0, 0, c_fl)

#define SETTINGS_STORAGE_KEY "settings"

static const char *servo_zero_degree_pwm_table[] = {
    "Min PWM",
    "Max PWM",
};
_Static_assert(ARRAY_COUNT(servo_zero_degree_pwm_table) == SERVO_ZERO_DEGREE_PWM_COUNT, "SERVO_ZERO_DEGREE_PWM_COUNT is invalid");

#if defined(USE_SCREEN)
static const char *screen_brightness_table[] = {"Low", "Medium", "High"};
static const char *screen_autopoweroff_table[] = {"Disabled", "30 sec", "1 min", "5 min", "10 min"};
#endif

typedef setting_visibility_e (*setting_visibility_f)(folder_id_e folder, settings_view_e view_id, const setting_t *setting);
typedef int (*setting_dynamic_format_f)(char *buf, size_t size, const setting_t *setting, setting_dynamic_format_e fmt);

static setting_visibility_e setting_visibility_root(folder_id_e folder, settings_view_e view_id, const setting_t *setting)
{
    if (SETTING_IS(setting, SETTING_KEY_WIFI))
    {
        return SETTING_SHOW_IF_SCREEN(view_id);
    }
    return SETTING_VISIBILITY_SHOW;
}

static setting_value_t setting_values[SETTING_COUNT];
static bool setting_values_is_temp[SETTING_TEMP_COUNT];
static setting_value_t setting_temp_values[SETTING_TEMP_COUNT];

static const setting_t settings[] = {
    FOLDER("", "Settings", FOLDER_ID_ROOT, 0, setting_visibility_root),
    FOLDER(SETTING_KEY_WIFI, "Wifi", FOLDER_ID_WIFI, FOLDER_ID_ROOT, NULL),
    STRING_SETTING(SETTING_KEY_WIFI_SSID, "SSID", FOLDER_ID_WIFI),
    STRING_SETTING(SETTING_KEY_WIFI_PWD, "PWD", FOLDER_ID_WIFI),
    BOOL_SETTING(SETTING_KEY_WIFI_SMART_CONFIG, "Smart Config", SETTING_FLAG_EPHEMERAL, FOLDER_ID_WIFI, false),
    STRING_SETTING(SETTING_KEY_WIFI_IP, "IP", FOLDER_ID_WIFI),
    FOLDER(SETTING_KEY_SERVO, "Servo", FOLDER_ID_SERVO, FOLDER_ID_ROOT, NULL),
    U16_SETTING(SETTING_KEY_SERVO_COURSE, "Course", SETTING_FLAG_VALUE, FOLDER_ID_SERVO, 0, 359, 0),
        FOLDER(SETTING_KEY_SERVO_TILT, "Tilt", FOLDER_ID_TILT, FOLDER_ID_SERVO, NULL),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH, "Max PWM", SETTING_FLAG_VALUE, FOLDER_ID_TILT, 0, 3000, DEFAULT_SERVO_MAX_PLUSEWIDTH, 1),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH, "Min PWM", SETTING_FLAG_VALUE, FOLDER_ID_TILT, 0, 3000, DEFAULT_SERVO_MIN_PLUSEWIDTH, 2),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_TILT_MAX_DEGREE, "Max Degree", SETTING_FLAG_VALUE, FOLDER_ID_TILT, 0, 360, DEFAULT_SERVO_MAX_DEGREE, 3),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_TILT_MIN_DEGREE, "Min Degree", SETTING_FLAG_VALUE, FOLDER_ID_TILT, 0, 360, DEFAULT_SERVO_MIN_DEGREE, 4),
    U8_MAP_SETTING(SETTING_KEY_SERVO_TILT_ZERO_DEGREE_PLUSEWIDTH, "Zero PWM", 0, FOLDER_ID_TILT, servo_zero_degree_pwm_table, MIN_PLUSEWIDTH),
    FOLDER(SETTING_KEY_SERVO_PAN, "Pan", FOLDER_ID_PAN, FOLDER_ID_SERVO, NULL),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH, "Max PWM", SETTING_FLAG_VALUE, FOLDER_ID_PAN, 0, 3000, DEFAULT_SERVO_MAX_PLUSEWIDTH, 5),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH, "Min PWM", SETTING_FLAG_VALUE, FOLDER_ID_PAN, 0, 3000, DEFAULT_SERVO_MIN_PLUSEWIDTH, 6),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_PAN_MAX_DEGREE, "Max Degree", SETTING_FLAG_VALUE, FOLDER_ID_PAN, 0, 360, DEFAULT_SERVO_MAX_DEGREE, 7),
    U16_HAS_TMP_SETTING(SETTING_KEY_SERVO_PAN_MIN_DEGREE, "Min Degree", SETTING_FLAG_VALUE, FOLDER_ID_PAN, 0, 360, DEFAULT_SERVO_MIN_DEGREE, 8),
    U8_MAP_SETTING(SETTING_KEY_SERVO_PAN_ZERO_DEGREE_PLUSEWIDTH, "Zero PWM", 0, FOLDER_ID_PAN, servo_zero_degree_pwm_table, MIN_PLUSEWIDTH),
#if defined(USE_SCREEN)
    FOLDER(SETTING_KEY_SCREEN, "Screen", FOLDER_ID_SCREEN, FOLDER_ID_ROOT, NULL),
    U8_MAP_SETTING(SETTING_KEY_SCREEN_BRIGHTNESS, "Brightness", 0, FOLDER_ID_SCREEN, screen_brightness_table, SCREEN_BRIGHTNESS_DEFAULT),
    U8_MAP_SETTING(SETTING_KEY_SCREEN_AUTO_OFF, "Auto Off", 0, FOLDER_ID_SCREEN, screen_autopoweroff_table, UI_SCREEN_AUTOOFF_DEFAULT),
#endif
    FOLDER(SETTING_KEY_DIAGNOSTICS, "Diagnostics", FOLDER_ID_DIAGNOSTICS, FOLDER_ID_ROOT, NULL),
    CMD_SETTING(SETTING_KEY_DIAGNOSTICS_DEBUG_INFO, "Debug Info", FOLDER_ID_DIAGNOSTICS, 0, 0),
    FOLDER(SETTING_KEY_DEVELOPER, "Developer Options", FOLDER_ID_DEVELOPER, FOLDER_ID_DIAGNOSTICS, NULL),
    CMD_SETTING(SETTING_KEY_DEVELOPER_REBOOT, "Reboot", FOLDER_ID_DEVELOPER, 0, 0),
};

_Static_assert(SETTING_COUNT == ARRAY_COUNT(settings), "SETTING_COUNT != ARRAY_COUNT(settings)");

typedef struct settings_listener_s
{
    setting_changed_f callback;
    void *user_data;
} settings_listener_t;

static settings_listener_t listeners[4];
static storage_t storage;

static void map_setting_keys(settings_view_t *view, const char *keys[], int size)
{
    view->count = 0;
    for (int ii = 0; ii < size; ii++)
    {
        int idx;
        ASSERT(settings_get_key_idx(keys[ii], &idx));
        view->indexes[view->count++] = (uint8_t)idx;
    }
}

static setting_value_t *setting_get_val_ptr(const setting_t *setting)
{
    int index = setting - settings;
    return &setting_values[index];
}

static setting_value_t *setting_get_tmp_val_ptr(const setting_t *setting)
{
    return &setting_temp_values[setting->tmp_index - 1];
}

static char *setting_get_str_ptr(const setting_t *setting)
{
    if (setting->type == SETTING_TYPE_STRING)
    {
        if (setting->flags & SETTING_FLAG_READONLY)
        {
            // Stored in the data ptr in the setting
            return (char *)setting->data;
        }
        // The u8 stored the string index
        return settings_string_storage[setting_get_val_ptr(setting)->u8];
    }
    return NULL;
}

static void setting_save(const setting_t *setting)
{
    if (setting->flags & SETTING_FLAG_EPHEMERAL)
    {
        return;
    }
    switch (setting->type)
    {
    case SETTING_TYPE_U8:
        storage_set_u8(&storage, setting->key, setting_get_val_ptr(setting)->u8);
        break;
    case SETTING_TYPE_I8:
        storage_set_i8(&storage, setting->key, setting_get_val_ptr(setting)->i8);
        break;
    case SETTING_TYPE_U16:
        storage_set_u16(&storage, setting->key, setting_get_val_ptr(setting)->u16);
        break;
    case SETTING_TYPE_I16:
        storage_set_i16(&storage, setting->key, setting_get_val_ptr(setting)->i16);
        break;
    case SETTING_TYPE_U32:
        storage_set_u32(&storage, setting->key, setting_get_val_ptr(setting)->u32);
        break;
    case SETTING_TYPE_I32:
        storage_set_i32(&storage, setting->key, setting_get_val_ptr(setting)->i32);
        break;
    case SETTING_TYPE_STRING:
        storage_set_str(&storage, setting->key, setting_get_str_ptr(setting));
        break;
    case SETTING_TYPE_FOLDER:
        break;
    }
    storage_commit(&storage);
}

static void setting_changed(const setting_t *setting)
{
    LOG_I(TAG, "Setting %s changed", setting->key);
    for (int ii = 0; ii < ARRAY_COUNT(listeners); ii++)
    {
        if (listeners[ii].callback)
        {
            listeners[ii].callback(setting, listeners[ii].user_data);
        }
    }
    setting_save(setting);
}

static void setting_move(const setting_t *setting, int delta)
{
    if (setting->flags & SETTING_FLAG_READONLY)
    {
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_DEGREE) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_DEGREE) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_DEGREE) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_DEGREE))
    {
        if (!setting_get_value_is_tmp(setting))
        {
            setting_set_tmp_u16(setting, setting_get_u16(setting));
            setting_set_value_is_tmp(setting, true);
        }
        
        setting_set_tmp_u16(setting, setting_get_tmp_u16(setting) + delta);
        return;
    }

    switch (setting->type)
    {
    case SETTING_TYPE_U8:
    {
        uint8_t v;
        uint8_t ov = setting_get_val_ptr(setting)->u8;
        if (delta < 0 && ov == 0)
        {
            v = setting->max.u8;
        }
        else if (delta > 0 && ov == setting->max.u8)
        {
            v = 0;
        }
        else
        {
            v = ov + delta;
        }
        if (v != ov)
        {
            setting_get_val_ptr(setting)->u8 = v;
            setting_changed(setting);
        }
        break;
    }
    case SETTING_TYPE_U16:
    {
        uint16_t v;
        uint16_t ov = setting_get_val_ptr(setting)->u16;
        if (delta < 0 && ov == 0)
        {
            v = setting->max.u16;
        }
        else if (delta > 0 && ov == setting->max.u16)
        {
            v = 0;
        }
        else
        {
            v = ov + delta;
        }
        if (v != ov)
        {
            setting_get_val_ptr(setting)->u16 = v;
            setting_changed(setting);
        }
        break;
    }
    default:
        break;
    }
}

void settings_init(void)
{
    storage_init(&storage, SETTINGS_STORAGE_KEY);

    // Initialize GPIO names
    // for (int ii = 0; ii < HAL_GPIO_USER_COUNT; ii++)
    // {
    //     hal_gpio_t x = gpio_get_configurable_at(ii);
    //     hal_gpio_toa(x, gpio_name_storage[ii], sizeof(gpio_name_storage[ii]));
    //     gpio_names[ii] = gpio_name_storage[ii];
    // }

    unsigned string_storage_index = 0;

    for (int ii = 0; ii < ARRAY_COUNT(settings); ii++)
    {
        const setting_t *setting = &settings[ii];
        if (setting->tmp_index > 0)
            setting_values_is_temp[setting->tmp_index - 1] = false;
        // Checking this at compile time is tricky, since most strings are
        // assembled via macros. Do it a runtime instead, impact should be
        // pretty minimal.
        if (strlen(setting->key) > MAX_SETTING_KEY_LENGTH)
        {
            LOG_E(TAG, "Setting key '%s' is too long (%d, max is %d)", setting->key,
                  strlen(setting->key), MAX_SETTING_KEY_LENGTH);
            abort();
        }
        if (setting->flags & SETTING_FLAG_READONLY)
        {
            continue;
        }
        bool found = true;
        size_t size;
        switch (setting->type)
        {
        case SETTING_TYPE_U8:
            found = storage_get_u8(&storage, setting->key, &setting_get_val_ptr(setting)->u8);
            break;
        case SETTING_TYPE_I8:
            found = storage_get_i8(&storage, setting->key, &setting_get_val_ptr(setting)->i8);
            break;
        case SETTING_TYPE_U16:
            found = storage_get_u16(&storage, setting->key, &setting_get_val_ptr(setting)->u16);
            break;
        case SETTING_TYPE_I16:
            found = storage_get_i16(&storage, setting->key, &setting_get_val_ptr(setting)->i16);
            break;
        case SETTING_TYPE_U32:
            found = storage_get_u32(&storage, setting->key, &setting_get_val_ptr(setting)->u32);
            break;
        case SETTING_TYPE_I32:
            found = storage_get_i32(&storage, setting->key, &setting_get_val_ptr(setting)->i32);
            break;
        case SETTING_TYPE_STRING:
            assert(string_storage_index < ARRAY_COUNT(settings_string_storage));
            setting_get_val_ptr(setting)->u8 = string_storage_index++;
            size = sizeof(settings_string_storage[0]);
            if (!storage_get_str(&storage, setting->key, setting_get_str_ptr(setting), &size))
            {
                memset(setting_get_str_ptr(setting), 0, SETTING_STRING_BUFFER_SIZE);
            }
            // We can't copy the default_value over to string settings, otherwise
            // the pointer becomes NULL.
            found = true;
            break;
        case SETTING_TYPE_FOLDER:
            break;
        }
        if (!found && !(setting->flags & SETTING_FLAG_CMD))
        {
            memcpy(setting_get_val_ptr(setting), &setting->def_val, sizeof(setting->def_val));
        }
    }
}

void settings_add_listener(setting_changed_f callback, void *user_data)
{
    for (int ii = 0; ii < ARRAY_COUNT(listeners); ii++)
    {
        if (!listeners[ii].callback)
        {
            listeners[ii].callback = callback;
            listeners[ii].user_data = user_data;
            return;
        }
    }
    // Must increase listeners size
    UNREACHABLE();
}

void settings_remove_listener(setting_changed_f callback, void *user_data)
{
    for (int ii = 0; ii < ARRAY_COUNT(listeners); ii++)
    {
        if (listeners[ii].callback == callback && listeners[ii].user_data == user_data)
        {
            listeners[ii].callback = NULL;
            listeners[ii].user_data = NULL;
            return;
        }
    }
    // Tried to remove an unexisting listener
    UNREACHABLE();
}

const setting_t *settings_get_setting_at(int idx)
{
    return &settings[idx];
}

const setting_t *settings_get_key(const char *key)
{
    return settings_get_key_idx(key, NULL);
}

uint8_t settings_get_key_u8(const char *key)
{
    return setting_get_u8(settings_get_key(key));
}

int8_t settings_get_key_i8(const char *key)
{
    return setting_get_i8(settings_get_key(key));
}

uint16_t settings_get_key_u16(const char *key)
{
    return setting_get_u16(settings_get_key(key));
}

int16_t settings_get_key_i16(const char *key)
{
    return setting_get_i16(settings_get_key(key));
}

uint32_t settings_get_key_u32(const char *key)
{
    return setting_get_u32(settings_get_key(key));
}

int32_t settings_get_key_i32(const char *key)
{
    return setting_get_i32(settings_get_key(key));
}

hal_gpio_t settings_get_key_gpio(const char *key)
{
    return setting_get_gpio(settings_get_key(key));
}

bool settings_get_key_bool(const char *key)
{
    return setting_get_bool(settings_get_key(key));
}

const char *settings_get_key_string(const char *key)
{
    return setting_get_string(settings_get_key(key));
}

const setting_t *settings_get_key_idx(const char *key, int *idx)
{
    for (int ii = 0; ii < ARRAY_COUNT(settings); ii++)
    {
        if (STR_EQUAL(key, settings[ii].key))
        {
            if (idx)
            {
                *idx = ii;
            }
            return &settings[ii];
        }
    }
    return NULL;
}

const setting_t *settings_get_folder(folder_id_e folder)
{
    for (int ii = 0; ii < ARRAY_COUNT(settings); ii++)
    {
        if (setting_get_folder_id(&settings[ii]) == folder)
        {
            return &settings[ii];
        }
    }
    return NULL;
}

bool settings_is_folder_visible(settings_view_e view_id, folder_id_e folder)
{
    const setting_t *setting = settings_get_folder(folder);
    return setting && setting_is_visible(view_id, setting->key);
}

bool setting_is_visible(settings_view_e view_id, const char *key)
{
    const setting_t *setting = settings_get_key(key);
    if (setting)
    {
        setting_visibility_e visibility = SETTING_VISIBILITY_SHOW;
        if (setting->folder)
        {
            const setting_t *folder = NULL;
            for (int ii = 0; ii < ARRAY_COUNT(settings); ii++)
            {
                if (setting_get_folder_id(&settings[ii]) == setting->folder)
                {
                    folder = &settings[ii];
                    break;
                }
            }
            if (folder && folder->data)
            {
                setting_visibility_f vf = folder->data;
                visibility = vf(setting_get_folder_id(folder), view_id, setting);
            }
        }
        return visibility != SETTING_VISIBILITY_HIDE;
    }
    return false;
}

folder_id_e setting_get_folder_id(const setting_t *setting)
{
    if (setting->type == SETTING_TYPE_FOLDER)
    {
        return setting->def_val.u8;
    }
    return 0;
}

folder_id_e setting_get_parent_folder_id(const setting_t *setting)
{
    return setting->folder;
}

int32_t setting_get_min(const setting_t *setting)
{
    switch (setting->type)
    {
    case SETTING_TYPE_U8:
        return setting->min.u8;
    case SETTING_TYPE_I8:
        return setting->min.i8;
    case SETTING_TYPE_U16:
        return setting->min.u16;
    case SETTING_TYPE_I16:
        return setting->min.i16;
    case SETTING_TYPE_U32:
        return setting->min.u32;
    case SETTING_TYPE_I32:
        return setting->min.i32;
    case SETTING_TYPE_STRING:
        break;
    case SETTING_TYPE_FOLDER:
        break;
    }
    return 0;
}

int32_t setting_get_max(const setting_t *setting)
{
    switch (setting->type)
    {
    case SETTING_TYPE_U8:
        return setting->max.u8;
    case SETTING_TYPE_I8:
        return setting->max.i8;
    case SETTING_TYPE_U16:
        return setting->max.u16;
    case SETTING_TYPE_I16:
        return setting->max.i16;
    case SETTING_TYPE_U32:
        return setting->max.u32;
    case SETTING_TYPE_I32:
        return setting->max.i32;
    case SETTING_TYPE_STRING:
        break;
    case SETTING_TYPE_FOLDER:
        break;
    }
    return 0;
}

int32_t setting_get_default(const setting_t *setting)
{
    switch (setting->type)
    {
    case SETTING_TYPE_U8:
        return setting->def_val.u8;
    case SETTING_TYPE_I8:
        return setting->def_val.i8;
    case SETTING_TYPE_U16:
        return setting->def_val.u16;
    case SETTING_TYPE_I16:
        return setting->def_val.i16;
    case SETTING_TYPE_U32:
        return setting->def_val.u32;
    case SETTING_TYPE_I32:
        return setting->def_val.i32;
    case SETTING_TYPE_STRING:
        break;
    case SETTING_TYPE_FOLDER:
        break;
    }
    return 0;
}

uint8_t setting_get_u8(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_U8);
    return setting_get_val_ptr(setting)->u8;
}

// Commands need special processing when used via setting values.
// This is only used when exposing the settings via CRSF.
static void setting_set_u8_cmd(const setting_t *setting, uint8_t v)
{
    setting_cmd_flag_e cmd_flags = setting_cmd_get_flags(setting);
    setting_value_t *val = setting_get_val_ptr(setting);
    switch ((setting_cmd_status_e)v)
    {
    case SETTING_CMD_STATUS_CHANGE:
        if (cmd_flags & SETTING_CMD_FLAG_CONFIRM)
        {
            // Setting needs a confirmation. Change its value to SETTING_CMD_STATUS_ASK_CONFIRM
            // So clients know they need to show the dialog.
            val->u8 = SETTING_CMD_STATUS_ASK_CONFIRM;
            break;
        }
        if (cmd_flags & SETTING_CMD_FLAG_WARNING)
        {
            val->u8 = SETTING_CMD_STATUS_SHOW_WARNING;
            break;
        }
        // TODO: Timeout if the client doesn't commit or discard after some time for
        // SETTING_CMD_FLAG_CONFIRM and SETTING_CMD_FLAG_WARNING

        // No flags. Just run it.
        setting_cmd_exec(setting);
        break;
    case SETTING_CMD_STATUS_COMMIT:
        setting_cmd_exec(setting);
        val->u8 = 0;
        break;
    case SETTING_CMD_STATUS_NONE:
    case SETTING_CMD_STATUS_DISCARD:
        // TODO: If the command shows a warning while it's active,
        // we need to generate a notification when it stops.
        val->u8 = 0;
        break;
    default:
        // TODO: Once we have a timeout, reset it here
        break;
    }
}

void setting_set_u8(const setting_t *setting, uint8_t v)
{
    assert(setting->type == SETTING_TYPE_U8);

    if (setting->flags & SETTING_FLAG_CMD)
    {
        setting_set_u8_cmd(setting, v);
        return;
    }

    v = MIN(v, setting->max.u8);
    v = MAX(v, setting->min.u8);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_val_ptr(setting)->u8 != v)
    {
        setting_get_val_ptr(setting)->u8 = v;
        setting_changed(setting);
    }
}

int8_t setting_get_i8(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_I8);
    return setting_get_val_ptr(setting)->i8;
}

void setting_set_i8(const setting_t *setting, int8_t v)
{
    assert(setting->type == SETTING_TYPE_I8);

    v = MIN(v, setting->max.i8);
    v = MAX(v, setting->min.i8);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_val_ptr(setting)->i8 != v)
    {
        setting_get_val_ptr(setting)->i8 = v;
        setting_changed(setting);
    }
}

uint16_t setting_get_u16(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_U16);
    return setting_get_val_ptr(setting)->u16;
}

void setting_set_u16(const setting_t *setting, uint16_t v)
{
    assert(setting->type == SETTING_TYPE_U16);

    v = MIN(v, setting->max.u16);
    v = MAX(v, setting->min.u16);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_val_ptr(setting)->u16 != v)
    {
        setting_get_val_ptr(setting)->u16 = v;
        setting_changed(setting);
    }
}

int16_t setting_get_i16(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_I16);
    return setting_get_val_ptr(setting)->i16;
}

void setting_set_i16(const setting_t *setting, int16_t v)
{
    assert(setting->type == SETTING_TYPE_I16);

    v = MIN(v, setting->max.i16);
    v = MAX(v, setting->min.i16);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_val_ptr(setting)->i16 != v)
    {
        setting_get_val_ptr(setting)->i16 = v;
        setting_changed(setting);
    }
}

uint32_t setting_get_u32(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_U32);
    return setting_get_val_ptr(setting)->u32;
}

void setting_set_u32(const setting_t *setting, uint32_t v)
{
    assert(setting->type == SETTING_TYPE_I32);

    v = MIN(v, setting->max.u32);
    v = MAX(v, setting->min.u32);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_val_ptr(setting)->u32 != v)
    {
        setting_get_val_ptr(setting)->u32 = v;
        setting_changed(setting);
    }
}

int32_t setting_get_i32(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_I32);
    return setting_get_val_ptr(setting)->i32;
}

void setting_set_i32(const setting_t *setting, int32_t v)
{
    assert(setting->type == SETTING_TYPE_I32);

    v = MIN(v, setting->max.i32);
    v = MAX(v, setting->min.i32);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_val_ptr(setting)->i32 != v)
    {
        setting_get_val_ptr(setting)->i32 = v;
        setting_changed(setting);
    }
}

// hal_gpio_t setting_get_gpio(const setting_t *setting)
// {
//     assert(setting->val_names == gpio_names);
//     return gpio_get_configurable_at(setting_get_val_ptr(setting)->u8);
// }

bool setting_get_bool(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_U8);
    return setting_get_val_ptr(setting)->u8 ? true : false;
}

void setting_set_bool(const setting_t *setting, bool v)
{
    assert(setting->type == SETTING_TYPE_U8);
    setting_set_u8(setting, v ? 1 : 0);
}

const char *setting_get_string(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_STRING);
    assert((setting->flags & SETTING_FLAG_DYNAMIC) == 0);
    return setting_get_str_ptr(setting);
}

void setting_set_string(const setting_t *setting, const char *s)
{
    assert(setting->type == SETTING_TYPE_STRING);
    if (setting->flags & SETTING_FLAG_READONLY)
    {
        return;
    }
    char *v = setting_get_str_ptr(setting);
    if (!STR_EQUAL(v, s))
    {
        strlcpy(v, s, SETTING_STRING_BUFFER_SIZE);
        setting_changed(setting);
    }
}

int settings_get_count(void)
{
    return ARRAY_COUNT(settings);
}

const char *setting_map_name(const setting_t *setting, uint8_t val)
{
    if (setting->flags & SETTING_FLAG_NAME_MAP && setting->val_names)
    {
        if (val <= setting->max.u8)
        {
            return setting->val_names[val];
        }
    }
    return NULL;
}

void setting_format_name(char *buf, size_t size, const setting_t *setting)
{
    if ((setting->flags & SETTING_FLAG_DYNAMIC) && setting->data)
    {
        setting_dynamic_format_f format_f = setting->data;
        if (format_f(buf, size, setting, SETTING_DYNAMIC_FORMAT_NAME) > 0)
        {
            return;
        }
    }
    if (setting->name)
    {
        strncpy(buf, setting->name, size);
    }
    else
    {
        buf[0] = '\0';
    }
}

void setting_format(char *buf, size_t size, const setting_t *setting)
{
    char name[SETTING_NAME_BUFFER_SIZE];

    setting_format_name(name, sizeof(name), setting);

    if (setting->type == SETTING_TYPE_FOLDER)
    {
        snprintf(buf, size, "%s >>", name);
        return;
    }
    if (setting->flags & SETTING_FLAG_CMD)
    {
        // Commands don't show their values
        snprintf(buf, size, "%s \xAC", name);
        return;
    }
    char value[64];
    setting_format_value(value, sizeof(value), setting);
    if (setting_get_value_is_tmp(setting))
    {
        snprintf(buf, size, "%s: %s->%d", name, value, setting_get_tmp_u16(setting));
    }
    else
    {
        snprintf(buf, size, "%s: %s", name, value);
    }
}

void setting_format_value(char *buf, size_t size, const setting_t *setting)
{
    if (setting->flags & SETTING_FLAG_NAME_MAP)
    {
        const char *name = setting_map_name(setting, setting_get_u8(setting));
        snprintf(buf, size, "%s", name);
    }
    else
    {
        switch (setting->type)
        {
        case SETTING_TYPE_U8:
            snprintf(buf, size, "%u", setting_get_u8(setting));
            break;
        case SETTING_TYPE_I8:
            snprintf(buf, size, "%d", setting_get_i8(setting));
            break;
        case SETTING_TYPE_U16:
            snprintf(buf, size, "%u", setting_get_u16(setting));
            break;
        case SETTING_TYPE_I16:
            snprintf(buf, size, "%d", setting_get_i16(setting));
            break;
        case SETTING_TYPE_U32:
            snprintf(buf, size, "%u", setting_get_u32(setting));
            break;
        case SETTING_TYPE_I32:
            snprintf(buf, size, "%d", setting_get_i32(setting));
            break;
        case SETTING_TYPE_STRING:
        {
            const char *value = setting_get_str_ptr(setting);
            char value_buf[SETTING_STRING_BUFFER_SIZE];
            if (setting->flags & SETTING_FLAG_DYNAMIC)
            {
                if (setting->data)
                {
                    setting_dynamic_format_f format_f = setting->data;
                    if (format_f(value_buf, sizeof(value_buf), setting, SETTING_DYNAMIC_FORMAT_VALUE) > 0)
                    {
                        value = value_buf;
                    }
                }
            }
            snprintf(buf, size, "%s", value ?: "<null>");
            break;
        }
        case SETTING_TYPE_FOLDER:
            break;
        }
    }
    if (setting->unit)
    {
        strlcat(buf, setting->unit, size);
    }
}

void setting_format_temp_value(char *buf, size_t size, const setting_t *setting)
{
    if (setting->flags & SETTING_FLAG_NAME_MAP)
    {
        const char *name = setting_map_name(setting, setting_get_u8(setting));
        snprintf(buf, size, "%s", name);
    }
    else
    {
        switch (setting->type)
        {
        case SETTING_TYPE_U8:
            snprintf(buf, size, "%u", setting_get_u8(setting));
            break;
        case SETTING_TYPE_I8:
            snprintf(buf, size, "%d", setting_get_i8(setting));
            break;
        case SETTING_TYPE_U16:
            snprintf(buf, size, "%u", setting_get_u16(setting));
            break;
        case SETTING_TYPE_I16:
            snprintf(buf, size, "%d", setting_get_i16(setting));
            break;
        case SETTING_TYPE_U32:
            snprintf(buf, size, "%u", setting_get_u32(setting));
            break;
        case SETTING_TYPE_I32:
            snprintf(buf, size, "%d", setting_get_i32(setting));
            break;
        case SETTING_TYPE_STRING:
        {
            const char *value = setting_get_str_ptr(setting);
            char value_buf[SETTING_STRING_BUFFER_SIZE];
            if (setting->flags & SETTING_FLAG_DYNAMIC)
            {
                if (setting->data)
                {
                    setting_dynamic_format_f format_f = setting->data;
                    if (format_f(value_buf, sizeof(value_buf), setting, SETTING_DYNAMIC_FORMAT_VALUE) > 0)
                    {
                        value = value_buf;
                    }
                }
            }
            snprintf(buf, size, "%s", value ?: "<null>");
            break;
        }
        case SETTING_TYPE_FOLDER:
            break;
        }
    }
    if (setting->unit)
    {
        strlcat(buf, setting->unit, size);
    }
}

setting_cmd_flag_e setting_cmd_get_flags(const setting_t *setting)
{
    if (setting->flags & SETTING_FLAG_CMD)
    {
        // CMD flags are stored in the default value
        return setting->def_val.u8;
    }
    return 0;
}

bool setting_cmd_exec(const setting_t *setting)
{
    setting_changed(setting);
    return true;
}

void setting_increment(const setting_t *setting)
{
    setting_move(setting, 1);
}

void setting_decrement(const setting_t *setting)
{
    setting_move(setting, -1);
}

static void settings_view_get_actual_folder_view(settings_view_t *view, settings_view_e view_id, folder_id_e folder, bool recursive)
{
    setting_visibility_f visibility_fn = NULL;
    for (int ii = 0; ii < SETTING_COUNT; ii++)
    {
        const setting_t *setting = &settings[ii];
        if (setting_get_folder_id(setting) == folder)
        {
            visibility_fn = settings[ii].data;
            continue;
        }
        if (setting_get_parent_folder_id(setting) == folder)
        {
            setting_visibility_e visibility = SETTING_VISIBILITY_SHOW;
            if (visibility_fn)
            {
                visibility = visibility_fn(folder, view_id, setting);
            }
            switch (visibility)
            {
            case SETTING_VISIBILITY_SHOW:
                view->indexes[view->count++] = ii;
                if (recursive && setting->type == SETTING_TYPE_FOLDER)
                {
                    // Include this dir
                    settings_view_get_actual_folder_view(view, view_id, setting_get_folder_id(setting), recursive);
                }
                break;
            case SETTING_VISIBILITY_HIDE:
                break;
            case SETTING_VISIBILITY_MOVE_CONTENTS_TO_PARENT:
                // Add the settings in this folder to its parent
                ASSERT(setting->type == SETTING_TYPE_FOLDER);
                settings_view_get_actual_folder_view(view, view_id, setting_get_folder_id(setting), recursive);
                break;
            }
        }
    }
}

bool settings_view_get_folder_view(settings_view_t *view, settings_view_e view_id, folder_id_e folder, bool recursive)
{
    view->count = 0;
    settings_view_get_actual_folder_view(view, view_id, folder, recursive);
    return view->count > 0;
}

bool settings_view_get(settings_view_t *view, settings_view_e view_id, folder_id_e folder)
{
    switch (view_id)
    {
    case SETTINGS_VIEW_CRSF_INPUT:
        //map_setting_keys(view, view_crsf_input_tx_settings, ARRAY_COUNT(view_crsf_input_tx_settings));
        return true;
    case SETTINGS_VIEW_MENU:
        return settings_view_get_folder_view(view, view_id, folder, false);
    case SETTINGS_VIEW_REMOTE:
        view->count = 0;
        return settings_view_get_folder_view(view, view_id, folder, true);
    }
    return false;
}

const setting_t *settings_view_get_setting_at(settings_view_t *view, int idx)
{
    if (idx >= 0 && idx < view->count)
    {
        return &settings[view->indexes[idx]];
    }
    return NULL;
}

int settings_view_get_parent_index(settings_view_t *view, const setting_t *setting)
{
    for (int ii = 0; ii < view->count; ii++)
    {
        const setting_t *vs = settings_view_get_setting_at(view, ii);
        if (vs->type == SETTING_TYPE_FOLDER && setting_get_folder_id(vs) == setting->folder)
        {
            return ii;
        }
    }
    return -1;
}

bool setting_get_value_is_tmp(const setting_t *setting)
{
    if (setting->tmp_index == 0)
        return false;
    assert(setting->tmp_index <= SETTING_TEMP_COUNT);
    return setting_values_is_temp[setting->tmp_index - 1];
}

void setting_set_value_is_tmp(const setting_t *setting, bool is_tmp)
{
    assert(setting->tmp_index > 0 && setting->tmp_index <= SETTING_TEMP_COUNT);
    setting_values_is_temp[setting->tmp_index - 1] = is_tmp;
}

uint16_t setting_get_tmp_u16(const setting_t *setting)
{
    assert(setting->type == SETTING_TYPE_U16);
    assert(setting->tmp_index > 0 && setting->tmp_index <= SETTING_TEMP_COUNT);
    return setting_get_tmp_val_ptr(setting)->u16;
}

void setting_set_tmp_u16(const setting_t *setting, uint16_t v)
{
    assert(setting->type == SETTING_TYPE_U16);
    assert(setting->tmp_index > 0 && setting->tmp_index <= SETTING_TEMP_COUNT);

    v = MIN(v, setting->max.u16);
    v = MAX(v, setting->min.u16);
    if ((setting->flags & SETTING_FLAG_READONLY) == 0 && setting_get_tmp_val_ptr(setting)->u16 != v)
    {
        setting_get_tmp_val_ptr(setting)->u16 = v;
    }
}