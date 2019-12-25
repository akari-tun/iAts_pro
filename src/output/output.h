#pragma once

#include <stdbool.h>

#include "util/time.h"

typedef struct output_vtable_s
{
    bool (*open)(void *output, void *config);
    bool (*update)(void *output, void *data, time_micros_t now);
    void (*close)(void *output, void *config);
} output_vtable_t;

typedef struct output_s
{
    bool is_open;
    void *data;
    output_vtable_t vtable;
} output_t;

bool output_open(void *data, output_t *output, void *config);
bool output_update(output_t *output, time_micros_t now);
void output_close(output_t *output, void *config);