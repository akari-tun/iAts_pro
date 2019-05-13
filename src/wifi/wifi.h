#include "esp_wifi.h"

#define DEFAULT_RSSI -127
#define CHAR_DOT '.'
#define BUFFER_LENGHT 512

typedef struct _Notifier notifier_t;

typedef enum 
{
    WIFI_STATUS_NONE = 0,
    WIFI_STATUS_SMARTCONFIG = 1,
    WIFI_STATUS_DISCONNECTED = 2,
    WIFI_STATUS_CONNECTING = 3,
    WIFI_STATUS_CONNECTED = 4,
    WIFI_STATUS_UDP_CONNECTED = 5,
} iats_wifi_status_e;

typedef void (*pTr_Analysis)(void *data, int offset, int len);
typedef void (*pTr_Send)(void *buffer, int len);
typedef void (*pTr_wifi_status_change)(void *w, uint8_t s);

typedef struct wifi_s
{
    bool reciving;
    iats_wifi_status_e status;
    char *buffer_received;
    uint32_t ip;

    wifi_config_t *config;
    pTr_Analysis callback;
    pTr_Send send;
    pTr_wifi_status_change status_change;

    notifier_t *status_change_notifier;
} wifi_t;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define SMART_CONFIG_START_BIT BIT2
#define SMART_CONFIG_DONE_BIT BIT3
#define UDP_CONNCETED_SUCCESS BIT4

void wifi_init(wifi_t *wifi);
void wifi_start(wifi_t *wifi);
void wifi_smartconfig_stop(wifi_t *wifi);
// void task_wifi(void *arg);