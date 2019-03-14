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

//#include "servo/servo.h"
#include "servo/servo_pwmc.h"

#include "ui/ui.h"
#include "ui/led.h"
#include "ui/beeper.h"

#include "util/time.h"
#include "util/macros.h"

//static const char *TAG = "MAIN";

//static servo_t servo;
static servo_pwmc_t servo;
static ui_t ui;

void iats_servo_init(void)
{
	ease_config_t e_cfg = {
		.steps = DEFAULT_EASE_STEPS,
		.min_pulsewidth = DEFAULT_EASE_MIN_PULSEWIDTH,
		.ease_out = DEFAULT_EASE_OUT,
	};

	servo.internal.ease_config = e_cfg;

	servo_pwmc_config_t s_cfg = {
		.min_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH,
		.max_pulsewidth = DEFAULT_SERVO_MAX_PLUSEWIDTH,
		.max_degree = DEFAULT_SERVO_MAX_DEGREE,
		.min_degree = DEFAULT_SERVO_MIN_DEGREE,
	};

	servo.internal.tilt.config = s_cfg;
	servo.internal.pan.config = s_cfg;
	servo_pwmc_init(&servo, &ui);
}

void task_servo_pwmc(void *arg)
{
	UNUSED(arg);

	time_ticks_t sleep;

	servo_pwmc_out(&servo.internal.pan, servo.internal.pan.config.max_pulsewidth);
	servo_pwmc_out(&servo.internal.tilt, servo.internal.pan.config.max_pulsewidth);
	vTaskDelay(MILLIS_TO_TICKS(3000));
	servo_pwmc_out(&servo.internal.pan, servo.internal.pan.config.min_pulsewidth);
	servo_pwmc_out(&servo.internal.tilt, servo.internal.pan.config.min_pulsewidth);
	
	bool isAdd = true;
	time_millis_t pan_tick = time_millis_now();
	time_millis_t tilt_tick = time_millis_now();

	time_millis_t wati = time_millis_now() + 5000;

	while (1)
	{
		if (time_millis_now() < wati)
		{
#if defined(USE_BEEPER)
			if (ui.internal.beeper.mode != BEEPER_MODE_WAIT_CONNECT)
			{
				beeper_set_mode(&ui.internal.beeper, BEEPER_MODE_WAIT_CONNECT);
			}
#endif
			if (!led_mode_is_enable(LED_MODE_WAIT_CONNECT))
			{
				led_mode_add(LED_MODE_WAIT_CONNECT);
			}
		}
		else
		{
			if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
			{
				ui.internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
				led_mode_remove(LED_MODE_WAIT_CONNECT);
			}

			if (time_millis_now() - tilt_tick > 240)
			{
				servo.internal.tilt.currtent_degree += isAdd ? 1 : -1;

				if (servo.internal.tilt.currtent_degree >= 90)
				{
					servo.internal.tilt.currtent_degree = 90;
					isAdd = false;
				}
				else if (servo.internal.tilt.currtent_degree <= 0)
				{
					servo.internal.tilt.currtent_degree = 0;
					isAdd = true;
				}

				servo.internal.tilt.is_reverse = servo.internal.pan.currtent_degree > 180;

				servo_pwmc_control(&servo.internal.tilt, &servo.internal.ease_config);

				tilt_tick = time_millis_now();
			}

			if (time_millis_now() - pan_tick > 100)
			{
				servo.internal.pan.currtent_degree += 1;
				if (servo.internal.pan.currtent_degree > 359 || servo.internal.pan.currtent_degree <= 0)
					servo.internal.pan.currtent_degree = 0;

				servo.internal.pan.is_reverse = servo.internal.pan.currtent_degree > 180;

				servo_pwmc_control(&servo.internal.pan, &servo.internal.ease_config);

				pan_tick = time_millis_now();
			}
		}

		// printf("[PAN pulse width: %dus degree: %d] [TILT pulse width: %dus degree: %d]\n",
		// 	   servo.internal.pan.currtent_pulsewidth, servo.internal.pan.currtent_degree,
		// 	   servo.internal.tilt.currtent_pulsewidth, servo.internal.tilt.currtent_degree);

		if (servo.internal.tilt.is_easing || servo.internal.pan.is_easing)
		{
			sleep = MILLIS_TO_TICKS(10);
			servo_pwmc_update(&servo);
		}
		else
		{
			sleep = MILLIS_TO_TICKS(100);
		}

		vTaskDelay(sleep);
	}
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

	ui_init(&ui, &cfg, &servo);
	led_mode_add(LED_MODE_BOOT);
}

void iats_battery_init(void)
{

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

void app_main()
{
	iats_ui_init();
	iats_servo_init();

	ui.internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
	ui.internal.screen.internal.secondary_mode = SCREEN_SECONDARY_MODE_NONE;

	xTaskCreatePinnedToCore(task_ui, "UI", 4096, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(task_servo_pwmc, "SERVO", 4096, NULL, 1, NULL, 1);
}