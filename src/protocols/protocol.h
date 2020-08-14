#define PROTOCOL_BAUDRATE_1200 1200
#define PROTOCOL_BAUDRATE_2400 2400
#define PROTOCOL_BAUDRATE_4800 4800
#define PROTOCOL_BAUDRATE_9600 9600
#define PROTOCOL_BAUDRATE_19200 19200
#define PROTOCOL_BAUDRATE_38400 38400
#define PROTOCOL_BAUDRATE_57600 57600
#define PROTOCOL_BAUDRATE_115200 115200

typedef enum
{
    PROTOCOL_ATP = 0,
    PROTOCOL_MSP,
    PROTOCOL_MAVLINK,
    PROTOCOL_LTM,
    PROTOCOL_NMEA,
    PROTOCOL_PELCO_D
} protocol_e;

typedef enum
{
    PROTOCOL_IO_INPUT = 0,
    PROTOCOL_IO_OUTPUT = 1,
} protocol_io_type_e;

typedef enum
{
    PROTOCOL_IO_ATP = 0,
    PROTOCOL_IO_BLUETOOTH,
    PROTOCOL_IO_UART1,
    PROTOCOL_IO_UART2
} protocol_io_source_e;