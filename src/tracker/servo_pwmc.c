#include "servo_pwmc.h"

#if defined(USE_PWMC)
#include <hal/pwm.h>
#include <hal/log.h>

#include "target/target.h"

static const char *TAG = "servo_pwmc";

void servo_pwm_initialize(servo_pwmc_t *servo)
{
    HAL_ERR_ASSERT_OK(hal_pwm_init());

    //pan
    servo_pwm_configuration(&servo->internal.pan, SERVO_PAN_GPIO);
    //tilt
    servo_pwm_configuration(&servo->internal.tilt, SERVO_TILT_GPIO);

    LOG_I(TAG, "servo gpio init [PAN:%d] [TILT:%d] ", servo->internal.pan.config.gpio, servo->internal.tilt.config.gpio);
}

void servo_pwm_configuration(servo_pwmc_status_t *status, hal_gpio_t gpio)
{
    servo_pwmc_config_t cfg = {
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

    status->step_positon = DEFAULT_EASE_MAX_STEPS;
    status->is_easing = false;
    status->is_reverse = false;

    HAL_ERR_ASSERT_OK(hal_pwm_open(gpio, SERVO_PWM_FREQUENCY_HZ, SERVO_PWM_RESOLUTION));
}

void servo_pwm_out(servo_pwmc_status_t *status, uint16_t pulsewidth)
{
    uint32_t v = (SERVO_PWM_MAX_VALUE - SERVO_PWM_MIN_VALUE) * (pulsewidth - status->config.min_pulsewidth);
    uint32_t duty = (v / (status->config.max_pulsewidth - status->config.min_pulsewidth)) + SERVO_PWM_MIN_VALUE;
    hal_pwm_set_duty(status->config.gpio, duty);
}
#endif