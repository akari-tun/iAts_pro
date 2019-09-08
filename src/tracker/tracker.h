#include <stdint.h>

#include "util/time.h"
#include "input/input_mavlink.h"
#include "telemetry.h"
#include "servo.h"
#include "observer.h"
#include "protocols/protocol.h"
// #include "protocols/atp.h"

// #if defined(USE_WIFI)
// #include "wifi/wifi.h"
// #endif

typedef struct _Notifier notifier_t;
typedef struct atp_s atp_t;

typedef enum
{
    TRACKER_STATUS_BOOTING = 1,
    TRACKER_STATUS_WIFI_SMART_CONFIG,
    TRACKER_STATUS_WIFI_CONNECTING,
    TRACKER_STATUS_WIFI_CONNECTED,
    TRACKER_STATUS_TRACKING,
    TRACKER_STATUS_MANUAL,
} tracker_status_e;

typedef enum
{
    TRACKER_FLAG_HOMESETED = 1,
    TRACKER_FLAG_PLANESETED = 2,
    TRACKER_FLAG_TRACKING = 4,
    TRACKER_FLAG_AUTO_NORTH = 8,
    TRACKER_FLAG_WIFI_CONNECTED = 16,
    TRACKER_FLAG_SERVER_CONNECTED = 32,
    TRACKER_FLAG_POWER_GOOD = 64,
} tracker_flag_e;

typedef enum
{
    TRACKER_MODE_AUTO = 1,
    TRACKER_MODE_MANUAL,
    TRACKER_MODE_DEBUG,
} tracker_mode_e;

typedef void (*pTr_status_changed)(void *t, tracker_status_e s);
typedef void (*pTr_flag_changed)(void *t, uint8_t f, uint8_t v);
typedef void (*pTr_telemetry_changed)(void *t, uint8_t tag);

typedef struct uart_s
{
    bool io_runing;
    bool invalidate_input;
    bool invalidate_output;
    hal_gpio_t gpio_tx;
    hal_gpio_t gpio_rx;
    int baudrate;
    protocol_e protocol;
    protocol_io_type_e io_type;

    union {
        input_mavlink_t mavlink;
    } inputs;

    void *input_config; 
    input_t *input;
} uart_t;

typedef struct tracker_s
{
    time_millis_t last_heartbeat;
    time_millis_t last_ack;

    struct
    {
        bool show_coordinate;
        tracker_flag_e flag;
        tracker_status_e status;
        // tracker_mode_e mode;
        pTr_status_changed status_changed;
        pTr_flag_changed flag_changed;
        pTr_telemetry_changed telemetry_changed;
        notifier_t *status_changed_notifier;
        notifier_t *flag_changed_notifier;
    } internal;

    uart_t uart1;
    uart_t uart2;
    // struct 
    // {
    //     bool io_runing;
    //     bool invalidate_input;
    //     bool invalidate_output;
    //     union {
    //         input_mavlink_t mavlink;
    //     } inputs;

    //     void *input_config; 
    //     input_t *input;
    // } io;
    
    servo_t *servo;
    atp_t *atp;
} tracker_t;

void tracker_init(tracker_t *t);
void tracker_uart_update(tracker_t *t, uart_t *uart);
void tracker_task(void *arg);
const char *telemetry_format_tracker_mode(const telemetry_t *val, char *buf, size_t bufsize);
tracker_status_e get_tracker_status(const tracker_t *t);
uint8_t get_tracker_flag(const tracker_t *t);
bool get_tracker_reversing(const tracker_t *t);
float get_plane_lat();
float get_plane_lon();
float get_plane_alt();
float get_tracker_lat();
float get_tracker_lon();
float get_tracker_alt();
void tracker_pan_move(tracker_t *t, int v);
void tracker_tilt_move(tracker_t *t, int v);