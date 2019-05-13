/* 
	A lot of the code for this project is copy from the raven(https://github.com/RavenLRS/raven)
*/
#include <stdio.h>

#include <hal/log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "target/target.h"

#include "ui/ui.h"
#include "ui/led.h"
#include "ui/beeper.h"
#include "wifi/wifi.h"
#include "config/settings.h"
#include "tracker/tracker.h"
#include "protocols/atp.h"

#include "util/time.h"
#include "util/macros.h"

// static const char *TAG = "MAIN";

static ui_t ui;
#if defined(USE_WIFI)
static wifi_t wifi;
#endif
static tracker_t tracker;

void iats_tracker_init(void)
{
	tracker_init(&tracker);
}

void iats_ui_init(void)
{
	ui_config_t cfg = {
		.buttons = {
			[BUTTON_ID_ENTER] = BUTTON_CONFIG_FROM_GPIO(BUTTON_ENTER_GPIO),
#if defined(USE_BUTTON_5WAY)
			[BUTTON_ID_LEFT] = BUTTON_CONFIG_FROM_GPIO(BUTTON_LEFT_GPIO),
			[BUTTON_ID_RIGHT] = BUTTON_CONFIG_FROM_GPIO(BUTTON_RIGHT_GPIO),
			[BUTTON_ID_UP] = BUTTON_CONFIG_FROM_GPIO(BUTTON_UP_GPIO),
			[BUTTON_ID_DOWN] = BUTTON_CONFIG_FROM_GPIO(BUTTON_DOWN_GPIO),
#endif
		},
#if defined(USE_BEEPER)
		.beeper = BEEPER_GPIO,
#endif
#ifdef USE_SCREEN
		.screen.i2c_bus = SCREEN_I2C_BUS,
		.screen.sda = SCREEN_GPIO_SDA,
		.screen.scl = SCREEN_GPIO_SCL,
		.screen.rst = SCREEN_GPIO_RST,
		.screen.addr = SCREEN_I2C_ADDR,
#endif
	};

#if defined(USE_WIFI)
	ui_init(&ui, &cfg, &tracker, &wifi);
#else
	ui_init(&ui, &cfg, &tracker);
#endif
	led_mode_add(LED_MODE_BOOT);
}

#if defined(USE_WIFI)
void iats_wifi_init(void)
{
	wifi_init(&wifi);
	wifi.callback = tracker.atp->atp_decode;
	tracker.atp->atp_send = wifi.send;
}
#endif

void task_ui(void *arg)
{
	UNUSED(arg);

	if (ui_screen_is_available(&ui))
	{
		ui_screen_splash(&ui);
	}
	
	for (;;)
	{
		ui_update(&ui);
		ui_yield(&ui);
	}
}

void app_main()
{
	settings_init();
	iats_tracker_init();
#if defined(USE_WIFI)
	iats_wifi_init();
#endif
	iats_ui_init();
	
	xTaskCreatePinnedToCore(task_tracker, "TRACKER", 4096, &tracker, 1, NULL, 0);
	xTaskCreatePinnedToCore(task_ui, "UI", 4096, NULL, 1, NULL, 1);
}