#pragma once

#include "io/io.h"
#include "util/data_state.h"
#include "tracker/telemetry.h"

#define PELCO_D_FRAME_SIZE 7
#define PELCO_D_SYNC_BYTE 0xFF
#define PELCO_D_ADDRESS 0x01
#define SET_PAN_POSITION 0x004B
#define SET_TILT_POSITION 0x004D

typedef struct pelco_d_s
{
    telemetry_t *plane_vals;
    telemetry_t *tracker_vals;
    io_t *io;
    uint8_t recive_buf[PELCO_D_FRAME_SIZE * 2];
    uint8_t send_buf[PELCO_D_FRAME_SIZE];
    int buf_pos;

    uint16_t pan_degree;
    uint16_t tilt_degree;
    
} pelco_d_t;

void pelco_d_init(pelco_d_t *pelco_d);
int pelco_d_update(pelco_d_t *pelco_d, void *data);
void pelco_d_destroy(pelco_d_t *pelco_d);