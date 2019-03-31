
typedef enum 
{
    WIFI_STATUS_NONE,
    WIFI_STATUS_SMARTCONFIG,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTED,
} iats_wifi_status_e;

typedef struct iats_wifi_config_s
{
    char *ssid;
    char *password;
} iats_wifi_config_t;

typedef void (*pTr_Analysis)(char *buffer[]);

typedef struct wifi_s
{
    iats_wifi_status_e status;
    char *buffer;
    char *ip;
    iats_wifi_config_t *config;
    pTr_Analysis callback;
} wifi_t;

#define WIFI_CONNECTED_BIT BIT0
#define UDP_CONNCETED_SUCCESS BIT1
#define ESPTOUCH_DONE_BIT BIT1

void wifi_init(wifi_t *wifi);
void task_wifi(void *arg);