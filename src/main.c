#include <stdio.h>

#include <hal/log.h>
// #include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "platform/system.h"

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

#include "io/hal_i2c.h"

#include "sensors/imu_task.h"

static ui_t ui;
#if defined(USE_WIFI)
static wifi_t wifi;
#endif
static tracker_t tracker;
static hal_i2c_config_t i2c_0_cfg;

static void setting_changed(const setting_t *setting, void *user_data)
{
    UNUSED(user_data);

    if (SETTING_IS(setting, SETTING_KEY_DEVELOPER_REBOOT))
    {
        system_reboot();
    }
}

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
		// .screen.i2c_bus = SCREEN_I2C_BUS,
		// .screen.sda = SCREEN_GPIO_SDA,
		// .screen.scl = SCREEN_GPIO_SCL,
		.screen.i2c_cfg = &i2c_0_cfg,
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
	wifi.t = tracker.atp;
	wifi.callback = tracker.atp->atp_decode;
	tracker.atp->atp_send = wifi.send;
}
#endif

void iats_i2c_init(void)
{
	i2c_0_cfg.i2c_bus = I2C_BUS;
	i2c_0_cfg.scl = I2C_GPIO_SCL;
	i2c_0_cfg.sda = I2C_GPIO_SDA;
	i2c_0_cfg.freq_hz = I2C_MASTER_FREQ_HZ;

	hal_i2c_init(&i2c_0_cfg);

	// i2c_1_cfg.i2c_bus = SCREEN_I2C_BUS;
	// i2c_1_cfg.scl = SCREEN_GPIO_SCL;
	// i2c_1_cfg.sda = SCREEN_GPIO_SDA;
	// i2c_1_cfg.freq_hz = SCREEN_I2C_MASTER_FREQ_HZ;

	// hal_i2c_init(&i2c_1_cfg);
}

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

void task_io(void *arg)
{
	if (settings_get_key_bool(SETTING_KEY_PORT_UART1_ENABLE) || settings_get_key_bool(SETTING_KEY_PORT_UART2_ENABLE))
	{
		tracker.uart1.io_runing = settings_get_key_bool(SETTING_KEY_PORT_UART1_ENABLE);
		tracker.uart2.io_runing = settings_get_key_bool(SETTING_KEY_PORT_UART2_ENABLE);

		while (tracker.uart1.io_runing || tracker.uart2.io_runing)
		{
			if (tracker.uart1.io_runing)
			{
				tracker_uart_update(&tracker, &tracker.uart1);
			}

			if (tracker.uart2.io_runing)
			{
				tracker_uart_update(&tracker, &tracker.uart2);
			}
		}
	}

	vTaskDelete(NULL);
}

void app_main()
{
	esp_log_level_set("*", ESP_LOG_INFO);

	settings_init();
	settings_add_listener(setting_changed, NULL);

	iats_i2c_init();
	iats_tracker_init();
#if defined(USE_WIFI)
	iats_wifi_init();
#endif
	iats_ui_init();

	xTaskCreatePinnedToCore(tracker_task, "TRACKER", 4096, &tracker, 1, NULL, 1);
	xTaskCreatePinnedToCore(task_ui, "UI", 4096, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(task_io, "TRACKER.IO", 4096, &tracker, 1, NULL, 1);

	//imu_task_init(&i2c_0_cfg);
	tracker.imu = imu_task_get();
}