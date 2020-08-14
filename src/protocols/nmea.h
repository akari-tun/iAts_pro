#include "io/io.h"
#include "util/data_state.h"
#include "../lib/gps_nmea_parser/include/gps/gps.h"
#include "tracker/telemetry.h"

#define NMEA_FRAME_SIZE_MAX 267

typedef struct nmea_s
{
    telemetry_t *plane_vals;
    io_t *io;
    uint8_t buf[NMEA_FRAME_SIZE_MAX * 2];
    int buf_pos;
    bool home_source;

    gps_t *gps;
    
} nmea_t;

void nmea_init(nmea_t *nmea);
int nmea_update(nmea_t *nmea, void *data);
void nmea_destroy(nmea_t *nmea);