#include <stdint.h>

#include "util/time.h"
#include "telemetry.h"
#include "servo.h"
#include "observer.h"
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
    TRACKER_STATUS_SERVER_CONNECTING,
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
} tracker_flag_e;

typedef enum
{
    TRACKER_MODE_AUTO = 1,
    TRACKER_MODE_MANUAL,
    TRACKER_MODE_DEBUG,
} tracker_mode_e;

typedef void (*pTr_status_changed)(void *t, tracker_status_e s);
typedef void (*pTr_flag_changed)(void *t, uint8_t f);
typedef void (*pTr_telemetry_changed)(void *t, uint8_t tag);

typedef struct tracker_s
{
    time_millis_t last_heartbeat;

    struct
    {
        uint8_t flag;
        tracker_status_e status;
        tracker_mode_e mode;
        pTr_status_changed status_changed;
        pTr_flag_changed flag_changed;
        pTr_telemetry_changed telemetry_changed;
        notifier_t *status_changed_notifier;
        notifier_t *flag_changed_notifier;
    } internal;

    servo_t *servo;
    atp_t *atp;
} tracker_t;

void tracker_init(tracker_t *t);
void task_tracker(void *arg);
const char *telemetry_format_tracker_mode(const telemetry_t *val, char *buf, size_t bufsize);
tracker_status_e get_tracker_status(const tracker_t *t);
uint8_t get_tracker_flag(const tracker_t *t);
bool get_tracker_reversing(const tracker_t *t);