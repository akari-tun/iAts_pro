#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
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

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
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
        //xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        LOG_I(TAG, "SYSTEM_EVENT_STA_START");
        
        if (wifi->status == WIFI_STATUS_NONE)
        {
            
            wifi_config_t wifi_config = {
                .sta = {
                    // .ssid = wifi->config->ssid,
                    // .password = wifi->config->password,
                    .scan_method = WIFI_FAST_SCAN,
                    .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                    .threshold.rssi = DEFAULT_RSSI,
                    .threshold.authmode = WIFI_AUTH_OPEN,
                }
            };

            strncpy(&wifi_config.sta.ssid, wifi->config->ssid, strlen(wifi->config->ssid));
            strncpy(&wifi_config.sta.password, wifi->config->password, strlen(wifi->config->password));

            LOG_I(TAG, "Connect to ap SSID:%s PWD:%s \n", wifi->config->ssid, wifi->config->password);

            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            wifi->status = WIFI_STATUS_CONNECTING;
        }
        else if (wifi->status == WIFI_STATUS_SMARTCONFIG)
        {
            ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
            ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
        }

        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        wifi->ip = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
        wifi->status = WIFI_STATUS_CONNECTED;
        ESP_LOGI(TAG, "Got IP: %s\n", wifi->ip);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        LOG_I(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        wifi->status = SYSTEM_EVENT_STA_DISCONNECTED;
        ESP_ERROR_CHECK(esp_wifi_connect());
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
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
    ESP_ERROR_CHECK( ret );

    const setting_t *setting_ssid = settings_get_key(SETTING_KEY_WIFI_SSID);
    const setting_t *setting_pwd = settings_get_key(SETTING_KEY_WIFI_PWD);

    iats_wifi_config_t iats_cfg = {
        .ssid = setting_get_string(setting_ssid),
        .password = setting_get_string(setting_pwd),
    };

    wifi->status = WIFI_STATUS_NONE;
    wifi->config = &iats_cfg;
    wifi->buffer = &buffer;

    // If wifi never configuration, use smartconfig to configuration wifi.
    if (iats_cfg.ssid != NULL && iats_cfg.password)
    {
        if (strlen(iats_cfg.ssid) == 0 || strlen(iats_cfg.password)) 
        {
            wifi->status = WIFI_STATUS_SMARTCONFIG;
        }
    }

    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, wifi));

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
}

void task_smartconfig(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "Smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL); 
        }

        vTaskDelay(MILLIS_TO_TICKS(10));
    }
}

void task_wifi(void *arg)
{
    wifi_t *wifi = arg;

	// wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = DEFAULT_SSID,
    //         .password = DEFAULT_PWD,
    //         .scan_method = WIFI_FAST_SCAN,
    //         .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
    //         .threshold.rssi = DEFAULT_RSSI,
    //         .threshold.authmode = WIFI_AUTH_OPEN,
    //     }
    // };
    // strncpy(&wifi_config.sta.ssid, wifi->config->ssid, strlen(wifi->config->ssid));
    // strncpy(&wifi_config.sta.password, wifi->config->password, strlen(wifi->config->password));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

    for (;;)
    {
        switch (wifi->status)
        {
        case WIFI_STATUS_SMARTCONFIG:
            break;
        case WIFI_STATUS_CONNECTING:
            break;
        case WIFI_STATUS_DISCONNECTED:
            break;
        default:
            break;
        }

        if (wifi->status != WIFI_STATUS_CONNECTING)
        {
            vTaskDelay(MILLIS_TO_TICKS(10));
        }
    }
}