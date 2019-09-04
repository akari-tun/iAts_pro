#pragma once

#include <stdbool.h>

#include "util/time.h"

typedef struct atp_s atp_t;

typedef struct input_vtable_s
{
    bool (*open)(void *input, void *config);
    bool (*update)(void *input, atp_t *atp, time_micros_t now);
    void (*close)(void *input, void *config);
} input_vtable_t;

typedef struct msp_transport_s msp_transport_t;

typedef struct input_s
{
    bool is_open;
    atp_t *atp;
    input_vtable_t vtable;
} input_t;

bool input_open(atp_t *atp, input_t *input, void *config);
bool input_update(input_t *input, time_micros_t now);
void input_close(input_t *input, void *config);