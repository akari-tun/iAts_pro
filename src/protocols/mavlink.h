#include "io/io.h"
#include "util/macros.h"
#include "util/time.h"
#include "util/data_state.h"
#include "tracker/telemetry.h"

#define MAVLINK_FRAME_SIZE_MAX 267

typedef struct __mavlink_status mavlink_status_t;
typedef struct __mavlink_message mavlink_message_t;
typedef struct __mavlink_global_position_int_t mavlink_global_position_int_t;

typedef struct mavlink_s
{
    telemetry_t *plane_vals;
    io_t *io;
    uint8_t buf[MAVLINK_FRAME_SIZE_MAX * 2];
    int buf_pos;
    mavlink_status_t *status;
    mavlink_message_t *message;
    uint8_t counter;
    float link_quality;

    struct 
    {
        mavlink_global_position_int_t *global_position;
    } message_value;
    
} mavlink_t;

void mavlink_init(mavlink_t *mavlink, io_t *io);
int mavlink_update(mavlink_t *mavlink, void *data);