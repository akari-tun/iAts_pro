#include "io/io.h"
#include "util/macros.h"
#include "util/time.h"
#include "util/data_state.h"
#include "tracker/telemetry.h"

typedef struct msp_s
{
    telemetry_t *plane_vals;
    io_t *io;
    uint8_t counter;
    float link_quality;
} msp_t;

void msp_init(msp_t *msp);
int msp_update(msp_t *msp, void *data);
void msp_destroy(msp_t *msp);