#include "io/io.h"

typedef enum
{
    PROTOCOL_ATP = 0,
    PROTOCOL_MSP,
    PROTOCOL_MAVLINK,
    PROTOCOL_COUNT,
} protocol_type_e;

typedef struct protocol_s
{
    protocol_type_e type;
    io_t *io;
} protocol_t;