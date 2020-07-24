#include <math.h>
#include <stdio.h>
#include <string.h>

#include "util/macros.h"

#include "telemetry.h"

const char *telemetry_format_str(const telemetry_t *val, char *buf, size_t bufsize)
{
    return val->val.s;
}

const char *telemetry_format_dbm(const telemetry_t *val, char *buf, size_t bufsize)
{
    int8_t dbm = val->val.i8;
    int mw = roundf(powf(10, dbm / 10.0));
    snprintf(buf, bufsize, "%dmW", mw);
    return buf;
}

const char *telemetry_format_db(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%ddB", val->val.i8);
    return buf;
}

const char *telemetry_format_snr(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%1.01fdB", val->val.i8 * 0.25f);
    return buf;
}

const char *telemetry_format_voltage(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02fV", val->val.u16 / 100.0);
    return buf;
}

const char *telemetry_format_current(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02fA", val->val.u16 / 100.0);
    return buf;
}

const char *telemetry_format_mah_i32(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%dmAh", val->val.i32);
    return buf;
}

const char *telemetry_format_mah_u16(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%umAh", val->val.u16);
    return buf;
}

const char *telemetry_format_spercentage(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%d%%", val->val.u8);
    return buf;
}

const char *telemetry_format_percentage(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%u%%", val->val.u8);
    return buf;
}

const char *telemetry_format_altitude(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02fm", val->val.i32 / 100.0);
    return buf;
}

const char *telemetry_format_vertical_speed(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02fm/s", val->val.i16 / 100.0);
    return buf;
}

const char *telemetry_format_deg(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%ddeg", val->val.u16 / 100);
    return buf;
}

const char *telemetry_format_acc(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02fG", val->val.i32 / 100.0);
    return buf;
}

const char *telemetry_format_att(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%+.02fdeg", val->val.i16 / 100.0);
    return buf;
}

const char *telemetry_format_ahrs(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%-5.2fdeg", val->val.f);
    return buf;
}


const char *telemetry_format_gps_fix(const telemetry_t *val, char *buf, size_t bufsize)
{
    switch (val->val.u8)
    {
    case 0:
        return "None";
    case 1:
        return "2D";
    case 2:
        return "3D";
    }
    return NULL;
}

const char *telemetry_format_u8(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%u", val->val.u8);
    return buf;
}

const char *telemetry_format_u16(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%d", val->val.u8);
    return buf;
}

const char *telemetry_format_coordinate(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.06f", val->val.i32 / 10000000.0);
    return buf;
}

const char *telemetry_format_horizontal_speed(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02fkm/h", (val->val.u16 / 100.0) * 3.6);
    return buf;
}

const char *telemetry_format_hdop(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%.02f", val->val.u16 / 100.0);
    return buf;
}

const char *telemetry_format_metre(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%dm", val->val.i32);
    return buf;
}

const char *telemetry_format_ip(const telemetry_t *val, char *buf, size_t bufsize)
{
    snprintf(buf, bufsize, "%d.%d.%d.%d", 
        (val->val.u32 >> 8 * 0) & 0xFF,
        (val->val.u32 >> 8 * 1) & 0xFF,
        (val->val.u32 >> 8 * 2) & 0xFF,
        (val->val.u32 >> 8 * 3) & 0xFF);
    return buf;
}

const char *telemetry_format_tracker_mode(const telemetry_t *val, char *buf, size_t bufsize)
{
    switch (val->val.u8)
    {
    case 1:
        return "Tracking";
    case 2:
        return "Manual";
    case 3:
        return "Debug";
    }
    return NULL;
}

bool telemetry_has_value(const telemetry_t *val)
{
    return data_state_get_last_update(&val->data_state) > 0;
}

bool telemetry_value_is_equal(const telemetry_t *val, const telemetry_val_t *new_val)
{
    switch (val->type)
    {
    case TELEMETRY_TYPE_UINT8:
        return val->val.u8 == new_val->u8;
    case TELEMETRY_TYPE_INT8:
        return val->val.i8 == new_val->i8;
    case TELEMETRY_TYPE_UINT16:
        return val->val.u16 == new_val->u16;
    case TELEMETRY_TYPE_INT16:
        return val->val.i16 == new_val->i16;
    case TELEMETRY_TYPE_UINT32:
        return val->val.u32 == new_val->u32;
    case TELEMETRY_TYPE_INT32:
        return val->val.i32 == new_val->i32;
    case TELEMETRY_TYPE_FLOAT:
        return val->val.f == new_val->f;
    case TELEMETRY_TYPE_STRING:
        return strcmp(val->val.s, new_val->s) == 0;
    }
    return false;
}

uint8_t telemetry_get_u8(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_UINT8);
    return val->val.u8;
}

int8_t telemetry_get_i8(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_INT8);
    return val->val.i8;
}

uint16_t telemetry_get_u16(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_UINT16);
    return val->val.u16;
}

int16_t telemetry_get_i16(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_INT16);
    return val->val.i16;
}

uint32_t telemetry_get_u32(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_UINT32);
    return val->val.u32;
}

int32_t telemetry_get_i32(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_INT32);
    return val->val.i32;
}

float telemetry_get_float(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_FLOAT);
    return val->val.f;
}


const char *telemetry_get_str(const telemetry_t *val)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_STRING);
    return val->val.s;
}

bool telemetry_set_u8(telemetry_t *val, uint8_t v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_UINT8);
    bool changed = v != val->val.u8;
    val->val.u8 = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_i8(telemetry_t *val, int8_t v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_INT8);
    bool changed = v != val->val.i8;
    val->val.i8 = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_u16(telemetry_t *val, uint16_t v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_UINT16);
    bool changed = v != val->val.u16;
    val->val.u16 = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_i16(telemetry_t *val, int16_t v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_INT16);
    bool changed = v != val->val.i16;
    val->val.i16 = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_u32(telemetry_t *val, uint32_t v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_UINT32);
    bool changed = v != val->val.u32;
    val->val.u32 = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_i32(telemetry_t *val, int32_t v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_INT32);
    bool changed = v != val->val.i32;
    val->val.i32 = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_float(telemetry_t *val, float v, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_FLOAT);
    bool changed = v != val->val.f;
    val->val.f = v;
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_str(telemetry_t *val, const char *str, time_micros_t now)
{
    TELEMETRY_ASSERT_TYPE(val->type, TELEMETRY_TYPE_STRING);
    const char empty = '\0';
    bool changed = false;
    if (!str)
    {
        str = &empty;
    }
    if (strcmp(str, val->val.s) != 0)
    {
        strncpy(val->val.s, str, sizeof(val->val.s));
        val->val.s[sizeof(val->val.s) - 1] = '\0';
        changed = true;
    }
    data_state_update(&val->data_state, changed, now);
    return changed;
}

bool telemetry_set_bytes(telemetry_t *val, const void *data, size_t size, time_micros_t now)
{
    bool changed = false;
    if (memcmp(&val->val, data, size) != 0)
    {
        memcpy(&val->val, data, size);
        changed = true;
    }
    data_state_update(&val->data_state, changed, now);
    return changed;
}
