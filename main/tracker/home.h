#pragma once

typedef struct home_s
{
    float latitude;
    float longitude;
    int32_t altitude;
    uint16_t heading;
    bool seted;
    bool real_time;
    bool auto_course;
} home_t;

void home_init(home_t *home);
void home_update(home_t *home);
void home_save(home_t *home);