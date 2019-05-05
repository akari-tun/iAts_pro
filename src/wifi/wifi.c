#include "../src/target/target.h"

#ifdef USE_WIFI
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

#include <hal/log.h>
#include "util/time.h"
#include "wifi.h"
#include "udp.h"
#include "config/settings.h"

static const char *TAG = "Wifi";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
// static EventGroupHandle_t wifi_event_group;
//Allocate 512 bytes buffer for received
static volatile char buffer_received[BUFFER_LENGHT];
static volatile char buffer_send[BUFFER_LENGHT];


static void task_connect(void *arg)
{
    wifi_t *wifi = arg;
    ESP_ERROR_CHECK(wifi_create_udp_client());

    char *index = strrchr(wifi->ip, CHAR_DOT);
    char *broadcast_addr;
    broadcast_addr = (char*)malloc(index - wifi->ip + 3);
    strncpy(broadcast_addr, wifi->ip, index - wifi->ip + 1);
    strcat(broadcast_addr, "255");

    LOG_I(TAG, "Broadcast Address: %s", broadcast_addr);
    wifi_udp_set_server_ip(broadcast_addr);

    free(broadcast_addr);
    int len = 0;
    char sendBuff[128] = "I want connect to you.";

    while (wifi->status == WIFI_STATUS_CONNECTED && wifi->status != WIFI_STATUS_UDP_CONNECTED)
    {    
        len = wifi_udp_send(sendBuff, strlen(sendBuff));
        vTaskDelay(MILLIS_TO_TICKS(5000));
    }
    
    vTaskDelete(NULL);
}

static void task_receive(void *arg)
{
    LOG_I(TAG, "Start receive task.");

    wifi_t *wifi = arg;
    ESP_ERROR_CHECK(wifi_create_udp_server());
    char *buffer;
    buffer = (char*)malloc(BUFFER_LENGHT);
    int len = 0;
    wifi->reciving = true;

    while (wifi->status == WIFI_STATUS_CONNECTED || wifi->status == WIFI_STATUS_UDP_CONNECTED)
    {    
        len = wifi_udp_receive(buffer, BUFFER_LENGHT);
        vTaskDelay(MILLIS_TO_TICKS(10));
    }

    wifi->reciving = false;
    LOG_I(TAG, "Stop receive task.");

    free(buffer);
    vTaskDelete(NULL);
}

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            LOG_I(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            LOG_I(TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            LOG_I(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            LOG_I(TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;

            const setting_t *setting_ssid = settings_get_key(SETTING_KEY_WIFI_SSID);
            const setting_t *setting_password = settings_get_key(SETTING_KEY_WIFI_PWD);

            setting_set_string(setting_ssid, (const char *)wifi_config->sta.ssid);
            setting_set_string(setting_password, (const char *)wifi_config->sta.password);

            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SC_STATUS_LINK_OVER:
            LOG_I(TAG, "SC_STATUS_LINK_OVER");
            // xEventGroupSetBits(wifi_event_group, SMART_CONFIG_DONE_BIT);
            LOG_I(TAG, "Smartconfig stop");
            esp_smartconfig_stop();
            break;
        default:
            break;
    }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    wifi_t *wifi = ctx;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:

        LOG_I(TAG, "SYSTEM_EVENT_STA_START");

        const char *ssid = setting_get_string(settings_get_key(SETTING_KEY_WIFI_SSID));
        const char *password = setting_get_string(settings_get_key(SETTING_KEY_WIFI_PWD));

        //If wifi never configuration, use smartconfig to configuration wifi.
        if (strlen(ssid) == 0 || strlen(password) == 0) 
        {
            wifi->status = WIFI_STATUS_SMARTCONFIG;
        }

        if (wifi->status == WIFI_STATUS_NONE)
        {
            wifi_config_t wifi_config = {
                .sta = {
                    .scan_method = WIFI_FAST_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = DEFAULT_RSSI,
                    .threshold.authmode = WIFI_AUTH_OPEN,
                },
            };

            // strcpy((char *)&wifi_config.sta.ssid, ssid);
            // strcpy((char *)&wifi_config.sta.password, password);
            strcpy((char *)&wifi_config.sta.ssid, "tan_wifi");
            strcpy((char *)&wifi_config.sta.password, "B048BEA833");
            
            LOG_I(TAG, "Connecting to ap SSID:%s PWD:%s", wifi_config.sta.ssid, wifi_config.sta.password);

            memcpy(wifi->config, &wifi_config, sizeof(wifi_config_t));

            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            wifi->status = WIFI_STATUS_CONNECTING;
        }
        else if (wifi->status == WIFI_STATUS_SMARTCONFIG)
        {
            LOG_I(TAG, "Smartconfig start");
            ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
            ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
        }
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        wifi->ip = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
        wifi_config_t cfg;
        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &cfg));
        memcpy(wifi->config, &cfg, sizeof(wifi_config_t));
        LOG_I(TAG, "SSID:%s PWD:%s IP:%s", wifi->config->sta.ssid, wifi->config->sta.password, wifi->ip);
        // xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        wifi->status = WIFI_STATUS_CONNECTED;
        xTaskCreatePinnedToCore(task_receive, "RECEIVE", 4096, wifi, 1, NULL, xPortGetCoreID());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        LOG_I(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());
        wifi->status = WIFI_STATUS_DISCONNECTED;
        // xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECTED_BIT);
        // xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init(wifi_t *wifi)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi->status = WIFI_STATUS_NONE;
    wifi->config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
    wifi->buffer_send = (char *)&buffer_send;
    wifi->buffer_received = (char *)&buffer_received;

    tcpip_adapter_init();
    // wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, wifi));

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    wifi_udp_init();
}

void wifi_start(wifi_t *wifi)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// static void event_bits_handle(EventBits_t uxBits, wifi_t *wifi)
// {
//     if(uxBits & SMART_CONFIG_DONE_BIT) {
//         LOG_I(TAG, "Smartconfig stop");
//         esp_smartconfig_stop();
//         wifi->status = WIFI_STATUS_CONNECTING;
//     }
//     if(uxBits & WIFI_CONNECTED_BIT) {
//         wifi->status = WIFI_STATUS_CONNECTED;
//     } 
//     if(uxBits & SMART_CONFIG_START_BIT) {
//         wifi->status = WIFI_STATUS_SMARTCONFIG;
//     } 
//     if(uxBits & WIFI_CONNECTED_BIT) {
//         wifi->status = WIFI_STATUS_CONNECTED;
//     }
//     if(uxBits & UDP_CONNCETED_SUCCESS) {
//         wifi->status = WIFI_STATUS_UDP_CONNECTED;
//     }
//     // if(uxBits & WIFI_DISCONNECTED_BIT) {
//     //     wifi->status = WIFI_STATUS_CONNECTING;
//     // }
// }

// void task_wifi(void *arg)
// {
//     wifi_t *wifi = arg;
    
// 	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
// 	ESP_ERROR_CHECK(esp_wifi_start());

//     EventBits_t uxBits;

//     TickType_t wait_delay = 0xff;

//     for (;;)
//     {
//         uxBits = 0;

//         switch (wifi->status)
//         {
//             case WIFI_STATUS_SMARTCONFIG:
//                 uxBits = xEventGroupWaitBits(wifi_event_group, 
//                     SMART_CONFIG_DONE_BIT, 
//                     true, false, wait_delay);
//                 break;
//             case WIFI_STATUS_CONNECTING:
//                 uxBits = xEventGroupWaitBits(wifi_event_group, 
//                     SMART_CONFIG_START_BIT | WIFI_CONNECTED_BIT, 
//                     true, false, wait_delay);
//                 break;
//             case WIFI_STATUS_CONNECTED:
//                 // xTaskCreatePinnedToCore(task_connect, "CONNECT", 4096, wifi, 1, NULL, xPortGetCoreID());
//                 if (!wifi->reciving)
//                 {
//                     xTaskCreatePinnedToCore(task_receive, "RECEIVE", 4096, wifi, 1, NULL, xPortGetCoreID());
//                 }
//                 uxBits = xEventGroupWaitBits(wifi_event_group, 
//                     UDP_CONNCETED_SUCCESS | WIFI_CONNECTED_BIT, 
//                     true, false, wait_delay); 
//                 break;
//             case WIFI_STATUS_DISCONNECTED:
//                 uxBits = xEventGroupWaitBits(wifi_event_group, 
//                     SMART_CONFIG_START_BIT | WIFI_CONNECTED_BIT, 
//                     true, false, wait_delay);
//                 wifi->status = WIFI_STATUS_CONNECTING;
//                 break;
//             case WIFI_STATUS_UDP_CONNECTED:
//                 break;
//             default:
//                 break;    
//         }

//         // if (wifi->status != WIFI_STATUS_CONNECTED)
//         // {
//             event_bits_handle(uxBits, wifi);
//             vTaskDelay(MILLIS_TO_TICKS(10));
//         // }
//     }
// }
#endif