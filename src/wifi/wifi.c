#include "../src/target/target.h"

#ifdef USE_WIFI
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

#include <hal/log.h>
#include "util/time.h"
#include "wifi.h"
#include "config/settings.h"

#define DEFAULT_RSSI -127

static const char *TAG = "Wifi";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
//Allocate 1024 bytes buffer for received
static volatile char buffer[1024];
//static iats_wifi_config_t *iats_wifi_confg;
//static wifi_t *wifi;

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
            // if (pdata != NULL) {
            //     uint8_t device_ip[4] = { 0 };
            //     memcpy(device_ip, (uint8_t* )pdata, 4);
            //     LOG_I(TAG, "Device ip: %d.%d.%d.%d\n", device_ip[0], device_ip[1], device_ip[2], device_ip[3]);
            // }
            xEventGroupSetBits(wifi_event_group, SMART_CONFIG_DONE_BIT);
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
                    // .ssid = "YC_WiFi",
                    // .password = "ycznyczn",
                    .scan_method = WIFI_FAST_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = DEFAULT_RSSI,
                    .threshold.authmode = WIFI_AUTH_OPEN,
                },
            };

            strcpy((char *)&wifi_config.sta.ssid, ssid);
            strcpy((char *)&wifi_config.sta.password, password);
            
            LOG_I(TAG, "Connecting to ap SSID:%s PWD:%s", wifi_config.sta.ssid, wifi_config.sta.password);

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
        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, wifi->config));
        LOG_I(TAG, "SSID:%s PWD:%s IP:%s", wifi->config->sta.ssid, wifi->config->sta.password, wifi->ip);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        LOG_I(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());
        wifi->status = WIFI_STATUS_DISCONNECTED;
        //xEventGroupClearBits(wifi_event_group, WIFI_DISCONNECTED_BIT);
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

    wifi_config_t cfg = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = DEFAULT_RSSI,
            .threshold.authmode = WIFI_AUTH_OPEN,
        }
    };

    wifi->status = WIFI_STATUS_NONE;
    wifi->config = &cfg;
    wifi->buffer = (char *)&buffer;

    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, wifi));

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
}

static void event_bits_handle(EventBits_t uxBits, wifi_t *wifi)
{
    if(uxBits & SMART_CONFIG_DONE_BIT) {
        LOG_I(TAG, "Smartconfig stop");
        esp_smartconfig_stop();
        wifi->status = WIFI_STATUS_CONNECTING;
    }
    if(uxBits & WIFI_CONNECTED_BIT) {
        wifi->status = WIFI_STATUS_CONNECTED;
    } 
    if(uxBits & SMART_CONFIG_START_BIT) {
        wifi->status = WIFI_STATUS_SMARTCONFIG;
    } 
    if(uxBits & WIFI_CONNECTED_BIT) {
        wifi->status = WIFI_STATUS_CONNECTED;
    }
}

void task_wifi(void *arg)
{
    wifi_t *wifi = arg;
    
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t uxBits;

    for (;;)
    {
        uxBits = 0;

        switch (wifi->status)
        {
            case WIFI_STATUS_SMARTCONFIG:
                uxBits = xEventGroupWaitBits(wifi_event_group, 
                    SMART_CONFIG_DONE_BIT, 
                    true, false, portMAX_DELAY);
                break;
            case WIFI_STATUS_CONNECTING:
                uxBits = xEventGroupWaitBits(wifi_event_group, 
                    WIFI_CONNECTED_BIT | WIFI_CONNECTED_BIT, 
                    true, false, portMAX_DELAY);
                break;
            case WIFI_STATUS_CONNECTED:
                // LOG_I(TAG, "SSID:%s PWD:%s IP:%s", wifi->config->sta.ssid, wifi->config->sta.password, wifi->ip);
                vTaskDelay(MILLIS_TO_TICKS(1000));
                break;
            case WIFI_STATUS_DISCONNECTED:
                uxBits = xEventGroupWaitBits(wifi_event_group, 
                    SMART_CONFIG_START_BIT | WIFI_CONNECTED_BIT, 
                    true, false, portMAX_DELAY); 
                break;
            default:
                break;    
        }

        if (wifi->status != WIFI_STATUS_CONNECTED)
        {
            event_bits_handle(uxBits, wifi);
            vTaskDelay(MILLIS_TO_TICKS(10));
        }
    }
}
#endif