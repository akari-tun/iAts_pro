#include "esp_wifi.h"

typedef enum 
{
    WIFI_STATUS_NONE,
    WIFI_STATUS_SMARTCONFIG,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTED,
} iats_wifi_status_e;

typedef void (*pTr_Analysis)(char *buffer[]);

typedef struct wifi_s
{
    iats_wifi_status_e status;
    char *buffer;
    char *ip;
    wifi_config_t *config;
    pTr_Analysis callback;
} wifi_t;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define SMART_CONFIG_START_BIT BIT2
#define SMART_CONFIG_DONE_BIT BIT3
#define UDP_CONNCETED_SUCCESS BIT4

void wifi_init(wifi_t *wifi);
void task_wifi(void *arg);