#pragma once

typedef struct plane_s
{
    float latitude;
    float longitude;
    int32_t altitude;
    bool seted;
} plane_t;

void plane_init(plane_t *plane);
void plane_update(plane_t *plane);