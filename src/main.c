/* servo motor control example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

// #include "mymath.h"

#include "ui/u8g2_esp32_hal.h"

#include "servo/servo.h"
#include "util/time.h"

static servo_t servo;

void task_servo(void *arg)
{
	//init status
	servo.servo_pan_status.currtent_degree = 0;
	servo.servo_pan_status.currtent_pulsewidth = 500;
	servo.servo_pan_status.is_easing = false;
	servo.servo_pan_status.is_reverse = false;
	servo.servo_pan_status.last_pulsewidth = 500;
	servo.servo_pan_status.step_positon = 0;

	servo.servo_tilt_status.currtent_degree = 0;
	servo.servo_tilt_status.currtent_pulsewidth = 500;
	servo.servo_tilt_status.is_easing = false;
	servo.servo_tilt_status.is_reverse = false;
	servo.servo_tilt_status.last_pulsewidth = 500;
	servo.servo_tilt_status.step_positon = 0;

	time_ticks_t sleep;

	while (1) {

		printf("[Tilt pulse width: %dus degree: %d]\n", servo.servo_tilt_status.currtent_pulsewidth, servo.servo_tilt_status.currtent_degree);

		servo_update(&servo);

		if (servo.servo_pan_status.is_easing || servo.servo_tilt_status.is_easing) {
			sleep = MILLIS_TO_TICKS(10);
		} else {
			sleep = MILLIS_TO_TICKS(100);
		}

        vTaskDelay(sleep);
	}
}

void task_degree(void *arg)
{
	bool isAdd = true;
	time_millis_t pan_tick = time_millis_now();
	time_millis_t tilt_tick = time_millis_now();

    while (1) {
		if (time_millis_now() - tilt_tick > 240) {
			servo.servo_tilt_status.currtent_degree += isAdd ? 1 : -1;

			if (servo.servo_tilt_status.currtent_degree >= 90) {
				servo.servo_tilt_status.currtent_degree = 90;
				isAdd = false;
			} else if (servo.servo_tilt_status.currtent_degree <= 0) {
				servo.servo_tilt_status.currtent_degree = 0;
				isAdd = true;
			}
			tilt_tick = time_millis_now();
		}

		if (time_millis_now() - pan_tick > 100) {
	 		servo.servo_pan_status.currtent_degree += 1;
	 		if (servo.servo_pan_status.currtent_degree > 359 || servo.servo_pan_status.currtent_degree <= 0) servo.servo_pan_status.currtent_degree = 0;
			pan_tick = time_millis_now();
		}

		time_ticks_t sleep = MILLIS_TO_TICKS(100);
        vTaskDelay(sleep);
    }
}

void app_main()
{
	//ease config
	servo.servo_ease_config.steps = 30;
	servo.servo_ease_config.min_pulsewidth = 700;
	servo.servo_ease_config.ease_out_type = EASE_OUT_QRT;

	//servo config
	servo.servo_mcpwm_config.mcpwm_unit = SERVO_MCPWM_UNIT;
	servo.servo_mcpwm_config.mcpwm_timer = SERVO_MCPWM_TIMER;
	servo.servo_mcpwm_config.mcpwm_frequency = SERVO_MCPWM_FREQUENCY;
	servo.servo_mcpwm_config.mcpwm_duty_type = SERVO_MCPWM_DUTY_MODE;
	servo.servo_mcpwm_config.mcpwm_counter_mode = SERVO_MCPWM_COUNTER_MODE;

	// pan config
	servo.servo_pan_config.mcpwm_io_signals = SERVO_PAN_MCPWM_IO_SIGNALS;
	servo.servo_pan_config.mcpwm_operator = SERVO_PAN_MCPWM_OPR;
	servo.servo_pan_config.servo_gpio = SERVO_PAN_GPIO;
	servo.servo_pan_config.servo_max_degree = 180;
	servo.servo_pan_config.servo_max_pulsewidth = 2500;
	servo.servo_pan_config.servo_mcpwm_config = &servo.servo_mcpwm_config;
	servo.servo_pan_config.servo_min_degree = 0;
	servo.servo_pan_config.servo_min_pulsewidth = 500;
	
	// tilt config
	servo.servo_tilt_config.mcpwm_io_signals = SERVO_TILT_MCPWM_IO_SIGNALS;
	servo.servo_tilt_config.mcpwm_operator = SERVO_TILT_MCPWM_OPR;
	servo.servo_tilt_config.servo_gpio = SERVO_TILT_GPIO;
	servo.servo_tilt_config.servo_max_degree = 180;
	servo.servo_tilt_config.servo_max_pulsewidth = 2500;
	servo.servo_tilt_config.servo_mcpwm_config = &servo.servo_mcpwm_config;
	servo.servo_tilt_config.servo_min_degree = 0;
	servo.servo_tilt_config.servo_min_pulsewidth = 500;

	servo_gpio_initialize(&servo);
	servo_configuration(&servo);

	xTaskCreate(task_servo, "task_servo", 4096, NULL, 1, NULL);
	xTaskCreate(task_degree, "task_degree", 4096, NULL, 2, NULL);
}