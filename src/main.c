/* 
	A lot of the code for this project is copy from the raven(https://github.com/RavenLRS/raven)
*/
#include <stdio.h>

#include <esp_log.h>
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

	bool isAdd = true;
	time_millis_t pan_tick = time_millis_now();
	time_millis_t tilt_tick = time_millis_now();

	while (1) {
		if (time_millis_now() - tilt_tick > 240) {
			servo.internal.tilt.currtent_degree += isAdd ? 1 : -1;

			if (servo.internal.tilt.currtent_degree >= 90) {
				servo.internal.tilt.currtent_degree = 90;
				isAdd = false;
			} else if (servo.internal.tilt.currtent_degree <= 0) {
				servo.internal.tilt.currtent_degree = 0;
				isAdd = true;
			}

			servo.internal.tilt.is_reverse = servo.internal.pan.currtent_degree > 180;

			servo_pwmc_control(&servo.internal.tilt, &servo.internal.ease_config);

			tilt_tick = time_millis_now();
		}

		if (time_millis_now() - pan_tick > 100) {
	 		servo.internal.pan.currtent_degree += 1;
	 		if (servo.internal.pan.currtent_degree > 359 || servo.internal.pan.currtent_degree <= 0) servo.internal.pan.currtent_degree = 0;

			servo.internal.pan.is_reverse = servo.internal.pan.currtent_degree > 180;
			
			servo_pwmc_control(&servo.internal.pan, &servo.internal.ease_config);

			pan_tick = time_millis_now();
		}

		printf("[PAN pulse width: %dus degree: %d] [TILT pulse width: %dus degree: %d]\n", 
			servo.internal.pan.currtent_pulsewidth, servo.internal.pan.currtent_degree,
			servo.internal.tilt.currtent_pulsewidth, servo.internal.tilt.currtent_degree);

		if (servo.internal.tilt.is_easing || servo.internal.pan.is_easing) {
			sleep = MILLIS_TO_TICKS(10);
			servo_pwmc_update(&servo);
		} else {
			sleep = MILLIS_TO_TICKS(100);
		}

        vTaskDelay(sleep);
	}
}

// void task_servo(void *arg)
// {
// 	//init status
// 	servo.servo_pan_status.currtent_degree = 0;
// 	servo.servo_pan_status.currtent_pulsewidth = 500;
// 	servo.servo_pan_status.is_easing = false;
// 	servo.servo_pan_status.is_reverse = false;
// 	servo.servo_pan_status.last_pulsewidth = 500;
// 	servo.servo_pan_status.step_positon = 0;

// 	servo.servo_tilt_status.currtent_degree = 0;
// 	servo.servo_tilt_status.currtent_pulsewidth = 500;
// 	servo.servo_tilt_status.is_easing = false;
// 	servo.servo_tilt_status.is_reverse = false;
// 	servo.servo_tilt_status.last_pulsewidth = 500;
// 	servo.servo_tilt_status.step_positon = 0;

// 	time_ticks_t sleep;

// 	bool isAdd = true;
// 	time_millis_t pan_tick = time_millis_now();
// 	time_millis_t tilt_tick = time_millis_now();

// 	while (1) {

// 		if (time_millis_now() - tilt_tick > 240) {
// 			servo.servo_tilt_status.currtent_degree += isAdd ? 1 : -1;

// 			if (servo.servo_tilt_status.currtent_degree >= 90) {
// 				servo.servo_tilt_status.currtent_degree = 90;
// 				isAdd = false;
// 			} else if (servo.servo_tilt_status.currtent_degree <= 0) {
// 				servo.servo_tilt_status.currtent_degree = 0;
// 				isAdd = true;
// 			}
// 			tilt_tick = time_millis_now();
// 		}

// 		if (time_millis_now() - pan_tick > 100) {
// 	 		servo.servo_pan_status.currtent_degree += 1;
// 	 		if (servo.servo_pan_status.currtent_degree > 359 || servo.servo_pan_status.currtent_degree <= 0) servo.servo_pan_status.currtent_degree = 0;
// 			pan_tick = time_millis_now();
// 		}

// 		printf("[PAN pulse width: %dus degree: %d] [TILT pulse width: %dus degree: %d]\n", 
// 			servo.servo_pan_status.currtent_pulsewidth, servo.servo_pan_status.currtent_degree,
// 			servo.servo_tilt_status.currtent_pulsewidth, servo.servo_tilt_status.currtent_degree);
			
// 		servo_update(&servo);

// 		if (servo.servo_pan_status.is_easing || servo.servo_tilt_status.is_easing) {
// 			sleep = MILLIS_TO_TICKS(10);
// 			// if (ui.internal.beeper.mode != BEEPER_MODE_EASING) {
// 			// 	beeper_set_mode(&ui.internal.beeper, BEEPER_MODE_EASING);
// 			// 	led_mode_add(LED_MODE_EASING);
// 			// }
// 		} else {
// 			sleep = MILLIS_TO_TICKS(100);
// 			// if (ui.internal.beeper.mode == BEEPER_MODE_NONE) {
// 			// 	led_mode_remove(LED_MODE_FAILSAFE);
// 			// }
// 		}

//         vTaskDelay(sleep);
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

    ui_init(&ui, &cfg);
    led_mode_add(LED_MODE_BOOT);
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
	//servo config

	// servo = {
	// 	.servo_ease_config = {
	// 		.steps = 30,
	// 		.min_pulsewidth = 700,
	// 		.ease_out_type = EASE_OUT_QRT,
	// 	},
	// 	.servo_mcpwm_config = {
	// 		.mcpwm_unit = SERVO_MCPWM_UNIT,
	// 		.mcpwm_timer = SERVO_MCPWM_TIMER,
	// 		.mcpwm_frequency = SERVO_MCPWM_FREQUENCY,
	// 		.mcpwm_duty_type = SERVO_MCPWM_DUTY_MODE,
	// 		.mcpwm_counter_mode = SERVO_MCPWM_COUNTER_MODE,
	// 	},
	// 	.servo_pan_config = {
	// 		.mcpwm_io_signals = SERVO_PAN_MCPWM_IO_SIGNALS,
	// 		.mcpwm_operator = SERVO_PAN_MCPWM_OPR,
	// 		.servo_gpio = SERVO_PAN_GPIO,
	// 		.servo_max_degree = 180,
	// 		.servo_max_pulsewidth = 2500,
	// 		.servo_mcpwm_config = &servo.servo_mcpwm_config,
	// 		.servo_min_degree = 0,
	// 		.servo_min_pulsewidth = 500,
	// 	},
	// 	.servo_tilt_config = {
	// 		.mcpwm_io_signals = SERVO_TILT_MCPWM_IO_SIGNALS,
	// 		.mcpwm_operator = SERVO_TILT_MCPWM_OPR,
	// 		.servo_gpio = SERVO_TILT_GPIO,
	// 		.servo_max_degree = 180,
	// 		.servo_max_pulsewidth = 2500,
	// 		.servo_mcpwm_config = &servo.servo_mcpwm_config,
	// 		.servo_min_degree = 0,
	// 		.servo_min_pulsewidth = 500,
	// 	},
	// };

	// servo.servo_ease_config.steps = 30;
	// servo.servo_ease_config.min_pulsewidth = 700;
	// servo.servo_ease_config.ease_out_type = EASE_OUT_QRT;

	// //servo config
	// servo.servo_mcpwm_config.mcpwm_unit = SERVO_MCPWM_UNIT;
	// servo.servo_mcpwm_config.mcpwm_timer = SERVO_MCPWM_TIMER;
	// servo.servo_mcpwm_config.mcpwm_frequency = SERVO_MCPWM_FREQUENCY;
	// servo.servo_mcpwm_config.mcpwm_duty_type = SERVO_MCPWM_DUTY_MODE;
	// servo.servo_mcpwm_config.mcpwm_counter_mode = SERVO_MCPWM_COUNTER_MODE;

	// // pan config
	// servo.servo_pan_config.mcpwm_io_signals = SERVO_PAN_MCPWM_IO_SIGNALS;
	// servo.servo_pan_config.mcpwm_operator = SERVO_PAN_MCPWM_OPR;
	// servo.servo_pan_config.servo_gpio = SERVO_PAN_GPIO;
	// servo.servo_pan_config.servo_max_degree = 180;
	// servo.servo_pan_config.servo_max_pulsewidth = 2500;
	// servo.servo_pan_config.servo_mcpwm_config = &servo.servo_mcpwm_config;
	// servo.servo_pan_config.servo_min_degree = 0;
	// servo.servo_pan_config.servo_min_pulsewidth = 500;
	
	// // tilt config
	// servo.servo_tilt_config.mcpwm_io_signals = SERVO_TILT_MCPWM_IO_SIGNALS;
	// servo.servo_tilt_config.mcpwm_operator = SERVO_TILT_MCPWM_OPR;
	// servo.servo_tilt_config.servo_gpio = SERVO_TILT_GPIO;
	// servo.servo_tilt_config.servo_max_degree = 180;
	// servo.servo_tilt_config.servo_max_pulsewidth = 2500;
	// servo.servo_tilt_config.servo_mcpwm_config = &servo.servo_mcpwm_config;
	// servo.servo_tilt_config.servo_min_degree = 0;
	// servo.servo_tilt_config.servo_min_pulsewidth = 500;

	// servo_gpio_initialize(&servo);
	// servo_configuration(&servo);

	iats_ui_init();
	iats_servo_init();

	xTaskCreatePinnedToCore(task_ui, "UI", 4096, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(task_servo_pwmc, "SERVO", 4096, NULL, 1, NULL, 1);
}