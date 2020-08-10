#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "time.h"

#include "util/data_state.h"
#include "util/macros.h"

#define TELEMETRY_STRING_MAX_SIZE 32

typedef enum
{
    TELEMETRY_TYPE_UINT8 = 1,
    TELEMETRY_TYPE_INT8,
    TELEMETRY_TYPE_UINT16,
    TELEMETRY_TYPE_INT16,
    TELEMETRY_TYPE_UINT32,
    TELEMETRY_TYPE_INT32,
    TELEMETRY_TYPE_FLOAT,
    TELEMETRY_TYPE_STRING, // null terminated
} telemetry_type_e;

#define TELEMETRY_UPLINK_COUNT 5

typedef union telemetry_u {
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    int16_t i16;
    uint32_t u32;
    int32_t i32;
    float f;
    char s[TELEMETRY_STRING_MAX_SIZE + 1];
} PACKED telemetry_val_t;

#define TELEMETRY_MAX_SIZE (sizeof(telemetry_val_t))
#define TELEMETRY_COUNT (TELEMETRY_UPLINK_COUNT + TELEMETRY_DOWNLINK_COUNT)

#define TELEMETRY_ASSERT_TYPE(t, typ) assert(t == typ)

typedef struct telemetry_s
{
    telemetry_type_e type;
    telemetry_val_t val;
    data_state_t data_state;
} telemetry_t;

const char *telemetry_format_str(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_dbm(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_db(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_snr(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_voltage(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_current(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_mah_i32(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_mah_u16(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_spercentage(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_percentage(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_altitude(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_vertical_speed(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_deg(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_acc(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_att(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_ahrs(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_gps_fix(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_u8(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_u16(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_u32(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_coordinate(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_horizontal_speed(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_hdop(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_metre(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_tracker_mode(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_ip(const telemetry_t *val, char *buf, size_t bufsize);
const char *telemetry_format_tracker_mode(const telemetry_t *val, char *buf, size_t bufsize);

int telemetry_get_id_count(void);
int telemetry_get_id_at(int idx);
// telemetry_type_e telemetry_get_type(int id);
// Returns 0 for variable sized types (e.g. string)
// size_t telemetry_get_data_size(int id);
// const char *telemetry_get_name(int id);
// const char *telemetry_format(const telemetry_t *val, int id, char *buf, size_t buf_size);
bool telemetry_has_value(const telemetry_t *val);

bool telemetry_value_is_equal(const telemetry_t *val, const telemetry_val_t *new_val);

uint8_t telemetry_get_u8(const telemetry_t *val);
int8_t telemetry_get_i8(const telemetry_t *val);
uint16_t telemetry_get_u16(const telemetry_t *val);
int16_t telemetry_get_i16(const telemetry_t *val);
uint32_t telemetry_get_u32(const telemetry_t *val);
int32_t telemetry_get_i32(const telemetry_t *val);
float telemetry_get_float(const telemetry_t *val);
const char *telemetry_get_str(const telemetry_t *val);
bool telemetry_set_u8(telemetry_t *val, uint8_t v, time_micros_t now);
bool telemetry_set_i8(telemetry_t *val, int8_t v, time_micros_t now);
bool telemetry_set_u16(telemetry_t *val, uint16_t v, time_micros_t now);
bool telemetry_set_i16(telemetry_t *val, int16_t v, time_micros_t now);
bool telemetry_set_u32(telemetry_t *val, uint32_t v, time_micros_t now);
bool telemetry_set_i32(telemetry_t *val, int32_t v, time_micros_t now);
bool telemetry_set_float(telemetry_t *val, float v, time_micros_t now);
bool telemetry_set_str(telemetry_t *val, const char *str, time_micros_t now);
bool telemetry_set_bytes(telemetry_t *val, const void *data, size_t size, time_micros_t now);
