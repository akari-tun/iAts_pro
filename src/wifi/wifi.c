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
// #include "esp_task_wdt.h"

#include <hal/log.h>
#include "util/time.h"
#include "wifi.h"
#include "udp.h"
#include "config/settings.h"
#include "tracker/observer.h"


static const char *TAG = "Wifi";

//Allocate 512 bytes buffer for received
static volatile char buffer_received[BUFFER_LENGHT];
//static volatile char buffer_send[BUFFER_LENGHT];

static const char *ip;

static void wifi_send(void *buffer, int len)
{
    wifi_udp_send(buffer, len);
}

static void task_receive(void *arg)
{
    LOG_I(TAG, "Start receive task at cpu core -> %d", xPortGetCoreID());

    wifi_t *wifi = arg;
    ESP_ERROR_CHECK(wifi_create_udp_server());
    ESP_ERROR_CHECK(wifi_create_udp_client());

    ip4_addr_t broadcast_addr = {
        .addr = (u32_t)(wifi->ip | 0xff000000)
    };

    LOG_I(TAG, "Broadcast Address: %s", ip4addr_ntoa(&broadcast_addr));
    wifi_udp_set_server_ip(&broadcast_addr.addr);

    char *buffer = (char *)&buffer_received;
    buffer = (char*)malloc(BUFFER_LENGHT);
    int len = 0;
    wifi->reciving = true;

    while (wifi->status == WIFI_STATUS_CONNECTED || wifi->status == WIFI_STATUS_UDP_CONNECTED)
    {    
        len = wifi_udp_receive(buffer, BUFFER_LENGHT);
        if (len > 0)
        {
            LOG_D(TAG, "Recving data length -> %d", len);
            wifi->callback(wifi->t, buffer, 0, len);
        }
        else
        {
            vTaskDelay(MILLIS_TO_TICKS(10));
        }
    }
    
    wifi->reciving = false;
    LOG_I(TAG, "Stop receive task.");

    // ESP_ERROR_CHECK(esp_wifi_disconnect());
    // LOG_I(TAG, "Stop wifi connect.");

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
            LOG_I(TAG, "Smartconfig stop");
            ESP_ERROR_CHECK(esp_smartconfig_stop());
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
            wifi->status_change(wifi, WIFI_STATUS_SMARTCONFIG);
        }

        if (wifi->status == WIFI_STATUS_CONNECTING)
        {
            wifi_config_t wifi_config = {
                .sta = {
                    .scan_method = WIFI_FAST_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = DEFAULT_RSSI,
                    .threshold.authmode = WIFI_AUTH_OPEN,
                },
            };

            strcpy((char *)&wifi_config.sta.ssid, ssid);
            strcpy((char *)&wifi_config.sta.password, password);
            // strcpy((char *)&wifi_config.sta.ssid, "tan_wifi");
            // strcpy((char *)&wifi_config.sta.password, "B04811BEA833");
            
            LOG_I(TAG, "Connecting to ap SSID:%s PWD:%s", wifi_config.sta.ssid, wifi_config.sta.password);

            memcpy(wifi->config, &wifi_config, sizeof(wifi_config_t));

            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            wifi->status_change(wifi, WIFI_STATUS_CONNECTING);
        }
        else if (wifi->status == WIFI_STATUS_SMARTCONFIG)
        {
            LOG_I(TAG, "Smartconfig start");
            ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
            ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
        }
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        wifi->ip = event->event_info.got_ip.ip_info.ip.addr;
        wifi_config_t cfg;
        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &cfg));
        memcpy(wifi->config, &cfg, sizeof(wifi_config_t));

        const char *ip_str = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
        // strcpy(ip, ip_str);
        setting_set_string(settings_get_key(SETTING_KEY_WIFI_IP), ip_str);

        LOG_I(TAG, "SSID:%s PWD:%s IP:%s", wifi->config->sta.ssid, wifi->config->sta.password, ip_str);
        wifi->status_change(wifi, WIFI_STATUS_CONNECTED);
        LOG_I(TAG, "Create Wifi receive task at Core:%d", xPortGetCoreID());
        xTaskCreatePinnedToCore(task_receive, "RECEIVE", 4096, wifi, 1, NULL, xPortGetCoreID());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        LOG_I(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        if (wifi->status == WIFI_STATUS_SMARTCONFIG)
        {
            LOG_I(TAG, "Smartconfig stop");
            ESP_ERROR_CHECK(esp_smartconfig_stop());
            ESP_ERROR_CHECK(esp_wifi_stop());
        }
        else
        {
            if (wifi->enable)
            {
                ESP_ERROR_CHECK(esp_wifi_connect());
                wifi->status_change(wifi, WIFI_STATUS_DISCONNECTED);
            }
            else
            {
                wifi->status_change(wifi, WIFI_STATUS_NONE);
            }
        }
        break;
    case SYSTEM_EVENT_STA_STOP:
        if (wifi->enable)
        {
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	        ESP_ERROR_CHECK(esp_wifi_start());
        }
        else
        {
            wifi->status_change(wifi, WIFI_STATUS_NONE);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void wifi_status_change(void *w, uint8_t status)
{
    wifi_t *wifi = (wifi_t*)w;
    
    LOG_I(TAG, "WIFI_STATUS_CHANGE -> %d", status);

    if (wifi->status == WIFI_STATUS_UDP_CONNECTED && status == WIFI_STATUS_CONNECTED)
    {
        ip4_addr_t broadcast_addr = {
            .addr = (u32_t)(wifi->ip | 0xff000000)
        };

        LOG_I(TAG, "Broadcast Address: %s", ip4addr_ntoa(&broadcast_addr));
        wifi_udp_set_server_ip(&broadcast_addr.addr);
    }

    wifi->status = status;
    wifi->status_change_notifier->mSubject.Notify(wifi->status_change_notifier, &wifi->status);
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

    ip = (char *)malloc(15);

    wifi->status_change = wifi_status_change;
    wifi->config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
    wifi->buffer_received = (char *)&buffer_received;
    wifi->send = wifi_send;
    wifi->status_change_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));

    const setting_t *wifi_enable_setting = settings_get_key(SETTING_KEY_WIFI_ENABLE);
    wifi->enable = setting_get_bool(wifi_enable_setting);

    wifi->status_change(wifi, WIFI_STATUS_CONNECTING);

    tcpip_adapter_init();
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

void wifi_stop(wifi_t *wifi)
{
    ESP_ERROR_CHECK(esp_smartconfig_stop());
    ESP_ERROR_CHECK(esp_wifi_stop());
}

void wifi_smartconfig_stop(wifi_t *wifi)
{
	ESP_ERROR_CHECK(esp_smartconfig_stop());
    ESP_ERROR_CHECK(esp_wifi_stop());
}
#endif