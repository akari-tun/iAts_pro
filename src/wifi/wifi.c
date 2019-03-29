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

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    wifi_t *wifi = ctx;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        //xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        LOG_I(TAG, "SYSTEM_EVENT_STA_START");
        //LOG_I(TAG, "Connect to ap SSID:%s PWD:%s \n", wifi->config->ssid, wifi->config->password);
        ESP_ERROR_CHECK(esp_wifi_connect());
        wifi->status = WIFI_STATUS_CONNECTING;
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

#define DEFAULT_SSID "YC_WIFI-2.4G"
#define DEFAULT_PWD "ycznyczn"

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

    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, wifi));

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
}

void task_wifi(void *arg)
{
    wifi_t *wifi = arg;

	wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = DEFAULT_RSSI,
            .threshold.authmode = WIFI_AUTH_OPEN,
        }
    };
    //strncpy(&wifi_config.sta.ssid, wifi->config->ssid, strlen(wifi->config->ssid));
    //strncpy(&wifi_config.sta.password, wifi->config->password, strlen(wifi->config->password));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
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