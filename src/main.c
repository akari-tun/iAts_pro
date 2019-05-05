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

// #include "tracker/servo.h"
//#include "servo/servo_pwmc.h"

#include "ui/ui.h"
#include "ui/led.h"
#include "ui/beeper.h"
#include "wifi/wifi.h"
#include "config/settings.h"
#include "tracker/tracker.h"

#include "util/time.h"
#include "util/macros.h"

// static const char *TAG = "MAIN";

static ui_t ui;
#if defined(USE_WIFI)
static wifi_t wifi;
#endif
static tracker_t tracker;

// void iats_servo_init(void)
// {
// 	ease_config_t e_cfg = {
// 		.steps = DEFAULT_EASE_STEPS,
// 		.min_pulsewidth = DEFAULT_EASE_MIN_PULSEWIDTH,
// 		.ease_out = DEFAULT_EASE_OUT,
// 	};

// 	servo.internal.ease_config = e_cfg;

// 	// servo_config_t s_cfg = {
// 	// 	.min_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH,
// 	// 	.max_pulsewidth = DEFAULT_SERVO_MAX_PLUSEWIDTH,
// 	// 	.max_degree = DEFAULT_SERVO_MAX_DEGREE,
// 	// 	.min_degree = DEFAULT_SERVO_MIN_DEGREE,
// 	// };

// 	// servo.internal.tilt.config.min_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH;
// 	// servo.internal.tilt.config.max_pulsewidth = DEFAULT_SERVO_MAX_PLUSEWIDTH;
// 	// servo.internal.tilt.config.max_degree = DEFAULT_SERVO_MAX_DEGREE;
// 	// servo.internal.tilt.config.min_degree = DEFAULT_SERVO_MIN_DEGREE;

// 	// servo.internal.tilt.config.min_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH;
// 	// servo.internal.tilt.config.max_pulsewidth = DEFAULT_SERVO_MAX_PLUSEWIDTH;
// 	// servo.internal.tilt.config.max_degree = DEFAULT_SERVO_MAX_DEGREE;
// 	// servo.internal.tilt.config.min_degree = DEFAULT_SERVO_MIN_DEGREE;

// 	servo_init(&servo, &ui);
// }

void iats_tracker_init(void)
{
	// tracker.wifi = &wifi;
	// tracker.ui = &ui;
	tracker_init(&tracker);
}

// void task_servo_pwmc(void *arg)
// {
// 	UNUSED(arg);

// 	time_ticks_t sleep;

// 	servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.max_pulsewidth);
// 	servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.max_pulsewidth);
// 	vTaskDelay(MILLIS_TO_TICKS(3000));
// 	servo_pulsewidth_out(&servo.internal.pan, servo.internal.pan.config.min_pulsewidth);
// 	servo_pulsewidth_out(&servo.internal.tilt, servo.internal.pan.config.min_pulsewidth);
	
// 	bool isAdd = true;
// 	time_millis_t pan_tick = time_millis_now();
// 	time_millis_t tilt_tick = time_millis_now();

// 	// time_millis_t wati = time_millis_now() + 5000;
	
// 	while (1)
// 	{
// 		if (wifi.status != WIFI_STATUS_CONNECTED)
// 		{
// #if defined(USE_BEEPER)
// 			if (ui.internal.beeper.mode != BEEPER_MODE_WAIT_CONNECT)
// 			{
// 				beeper_set_mode(&ui.internal.beeper, BEEPER_MODE_WAIT_CONNECT);
// 			}
// #endif
// 			if (!led_mode_is_enable(LED_MODE_WAIT_CONNECT))
// 			{
// 				led_mode_add(LED_MODE_WAIT_CONNECT);
// 			}
// 		}
// 		else
// 		{
// 			if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
// 			{
// 				ui.internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
// #if defined(USE_BEEPER)
// 				beeper_set_mode(&ui.internal.beeper, BEEPER_MODE_NONE);
// #endif
// 				led_mode_remove(LED_MODE_WAIT_CONNECT);
// 				//led_mode_add(LED_MODE_TRACKING);
// 			}

// 			if (time_millis_now() - tilt_tick > 80)
// 			{
// 				servo.internal.tilt.currtent_degree += isAdd ? 1 : -1;

// 				if (servo.internal.tilt.currtent_degree >= 90)
// 				{
// 					servo.internal.tilt.currtent_degree = 90;
// 					isAdd = false;
// 				}
// 				else if (servo.internal.tilt.currtent_degree <= 0)
// 				{
// 					servo.internal.tilt.currtent_degree = 0;
// 					isAdd = true;
// 				}

// 				servo.internal.tilt.is_reverse = servo.internal.pan.currtent_degree > 180;
// 				servo_pulsewidth_control(&servo.internal.tilt, &servo.internal.ease_config);

// 				ui.internal.led_gradual_target.tilt = servo.internal.tilt.currtent_degree;
// 				ui.internal.led_gradual_target.tilt_pulsewidth = servo.internal.tilt.currtent_pulsewidth;

// 				tilt_tick = time_millis_now();
// 			}

// 			if (time_millis_now() - pan_tick > 50)
// 			{
// 				servo.internal.pan.currtent_degree += 1;
// 				if (servo.internal.pan.currtent_degree > 359 || servo.internal.pan.currtent_degree <= 0)
// 					servo.internal.pan.currtent_degree = 0;

// 				servo.internal.pan.is_reverse = servo.internal.pan.currtent_degree > 180;
// 				servo_pulsewidth_control(&servo.internal.pan, &servo.internal.ease_config);

// 				ui.internal.led_gradual_target.pan = servo.internal.pan.currtent_degree;
// 				ui.internal.led_gradual_target.pan_pulsewidth = servo.internal.pan.currtent_pulsewidth;

// 				pan_tick = time_millis_now();
// 			}
// 		}

// 		// printf("[PAN pulse width: %dus degree: %d] [TILT pulse width: %dus degree: %d]\n",
// 		// 	   servo.internal.pan.currtent_pulsewidth, servo.internal.pan.currtent_degree,
// 		// 	   servo.internal.tilt.currtent_pulsewidth, servo.internal.tilt.currtent_degree);

// 		if (servo.internal.tilt.is_easing || servo.internal.pan.is_easing)
// 		{
// 			sleep = MILLIS_TO_TICKS(10);
// 			servo_update(&servo);
// 		}
// 		else
// 		{
// 			sleep = MILLIS_TO_TICKS(10);
// 			// if (time_millis_now() > wati)
// 			// {
// 			// 	led_mode_add(LED_MODE_TRACKING);
// 			// }
// 		}

// 		vTaskDelay(sleep);
// 	}
// }

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

	ui_init(&ui, &cfg, &tracker);
	led_mode_add(LED_MODE_BOOT);
}

#if defined(USE_WIFI)
void iats_wifi_init(void)
{
	wifi_init(&wifi);
	wifi.callback = tracker.internal.atp_decode;
	ui.internal.screen.internal.wifi = &wifi;
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
	iats_ui_init();
	// iats_servo_init();

	ui.internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
	ui.internal.screen.internal.secondary_mode = SCREEN_SECONDARY_MODE_NONE;
	
	xTaskCreatePinnedToCore(task_tracker, "TRACKER", 4096, &tracker, 1, NULL, 0);
	xTaskCreatePinnedToCore(task_ui, "UI", 4096, NULL, 1, NULL, 1);
#if defined(USE_WIFI)
	iats_wifi_init();
	// wifi.callback = tracker.internal.atp_decode;
	// ui.internal.screen.internal.wifi = &wifi;
	// xTaskCreatePinnedToCore(task_wifi, "WIFI", 4096, &wifi, 1, NULL, 1);
#endif
}