#include "servo_mcpwm.h"

#if defined(ESP32) && defined(USE_MCPWM)

#include <hal/log.h>

static const char *TAG = "servo";

void servo_pwm_initialize(servo_mcpwm_t *servo)
{
    servo_pwm_configuration(&servo->internal.pan, SERVO_PAN_GPIO);
    servo_pwm_configuration(&servo->internal.tilt, SERVO_TILT_GPIO);

    mcpwm_gpio_init(SERVO_MCPWM_UNIT, SERVO_PAN_MCPWM_IO_SIGNALS, servo->internal.pan.config.gpio);
    mcpwm_gpio_init(SERVO_MCPWM_UNIT, SERVO_TILT_MCPWM_IO_SIGNALS, servo->internal.tilt.config.gpio);

    LOG_I(TAG, "servo gpio init [PAN:%d] [TILT:%d] ", servo->internal.pan.config.gpio, servo->internal.tilt.config.gpio);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = SERVO_MCPWM_FREQUENCY;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 50;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 50;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = SERVO_MCPWM_COUNTER_MODE;
    pwm_config.duty_mode = SERVO_MCPWM_DUTY_MODE;

    mcpwm_init(SERVO_MCPWM_UNIT, SERVO_MCPWM_TIMER, &pwm_config);    //Configure PWM0A & PWM0B with above settings  
}

void servo_pwm_configuration(servo_mcpwm_status_t *status, hal_gpio_t gpio)
{
    servo_mcpwm_config_t cfg = {
        .gpio = gpio,
		.min_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH,
		.max_pulsewidth = DEFAULT_SERVO_MAX_PLUSEWIDTH,
		.max_degree = DEFAULT_SERVO_MAX_DEGREE,
		.min_degree = DEFAULT_SERVO_MIN_DEGREE,
	};

    status->config = cfg;
    status->currtent_degree = DEFAULT_SERVO_MIN_DEGREE;
    status->currtent_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH;
    status->last_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH;

    status->step_positon = DEFAULT_EASE_STEPS;
    status->is_easing = false;
    status->is_reverse = false;
}

void servo_pwm_out(servo_mcpwm_status_t *status, uint16_t pulsewidth)
{
    mcpwm_set_duty_in_us(SERVO_MCPWM_UNIT, 
        SERVO_MCPWM_TIMER, 
        status->config.gpio == SERVO_PAN_GPIO ? SERVO_PAN_MCPWM_OPR : SERVO_TILT_MCPWM_OPR, 
        pulsewidth);
}
#endif