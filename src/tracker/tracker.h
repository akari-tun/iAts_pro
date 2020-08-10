#include <stdint.h>

#include "util/time.h"
#include "input/input_mavlink.h"
#include "input/input_ltm.h"
#include "input/input_nmea.h"
#include "output/output_pelco_d.h"
#include "telemetry.h"
#include "servo.h"
#include "observer.h"
#include "protocols/protocol.h"
#include "sensors/imu.h"
// #include "protocols/atp.h"

// #if defined(USE_WIFI)
// #include "wifi/wifi.h"
// #endif

#define MAX_ESTIMATE_COUNT 5

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

typedef enum
{
    TRACKER_ESTIMATE_1_SEC = 1,
    TRACKER_ESTIMATE_3_SEC = 3,
    TRACKER_ESTIMATE_5_SEC = 5,
    TRACKER_ESTIMATE_10_SEC = 10
} tracker_estimate_e;

typedef void (*pTr_status_changed)(void *t, tracker_status_e s);
typedef void (*pTr_flag_changed)(void *t, uint8_t f, uint8_t v);
typedef void (*pTr_telemetry_changed)(void *t, uint8_t tag);

typedef struct uart_s
{
    uint8_t com;
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
        input_ltm_t ltm;
        input_nmea_t nmea;
    } inputs;

    union {
        output_pelco_d_t pelco_d;
    } outputs;

    void *input_config; 
    input_t *input;

    void *output_config;
    output_t *output;
} uart_t;

typedef struct location_estimate_s
{
    float latitude;
    float longitude;
    uint16_t direction;
    int16_t speed;
    time_millis_t location_time;
} location_estimate_t;

typedef struct tracker_s
{
    time_millis_t last_heartbeat;
    time_millis_t last_ack;

    struct
    {
        bool show_coordinate;
        bool real_alt;
        bool estimate_location;
        bool advanced_position;
        uint8_t eastimate_time;
        uint16_t advanced_time;
        tracker_flag_e flag;
        tracker_status_e status;
        // tracker_mode_e mode;
        pTr_status_changed status_changed;
        pTr_flag_changed flag_changed;
        pTr_telemetry_changed telemetry_changed;
        notifier_t *status_changed_notifier;
        notifier_t *flag_changed_notifier;
        location_estimate_t *estimate;
    } internal;

    uart_t uart1;
    uart_t uart2;
    
    servo_t *servo;
    atp_t *atp;
    imu_t *imu;
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
float get_plane_speed();
uint16_t get_plane_direction();
float get_tracker_lat();
float get_tracker_lon();
float get_tracker_alt();
float get_tracker_roll();
float get_tracker_pitch();
float get_tracker_yaw();
uint32_t get_tracker_imu_hz();
void tracker_pan_move(tracker_t *t, int v);
void tracker_tilt_move(tracker_t *t, int v);