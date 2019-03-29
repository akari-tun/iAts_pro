#include <hal/pwm.h>
#include <esp_log.h>

#include "servo_pwmc.h"

#include "target/target.h"

#include "ui/led.h"
#include "ui/beeper.h"

#if defined(USE_BEEPER)
    static beeper_t *beeper;
#endif

// #if defined(LED_1_USE_WS2812)
//     volatile static led_gradual_target_t *led_gradual_target;
// #endif

static const char *TAG = "servo_pwmc";

void servo_pwmc_init(servo_pwmc_t *servo_pwmc, ui_t *ui)
{
    HAL_ERR_ASSERT_OK(hal_pwm_init());

#if defined(USE_BEEPER)
    beeper = &ui->internal.beeper;
#endif

// #if defined(LED_1_USE_WS2812)
//     led_gradual_target = &ui->internal.led_gradual_target;
// #endif

    //pan
    servo_pwmc->internal.pan.pTr_pulsewidth_cal = &servo_pwmc_pan_per_degree_cal;
    servo_pwmc->internal.pan.gpio = SERVO_PAN_GPIO;
    servo_pwmc_config(&servo_pwmc->internal.pan, servo_pwmc->internal.pan.gpio);
    //tilt
    servo_pwmc->internal.tilt.pTr_pulsewidth_cal = &servo_pwmc_tilt_per_degree_cal;
    servo_pwmc->internal.tilt.gpio = SERVO_TILT_GPIO;
    servo_pwmc_config(&servo_pwmc->internal.tilt, servo_pwmc->internal.tilt.gpio);

    ESP_LOGI(TAG, "servo gpio init [PAN:%d] [TILT:%d] ", servo_pwmc->internal.pan.gpio, servo_pwmc->internal.tilt.gpio);
}

void servo_pwmc_config(servo_pwmc_status_t *status, hal_gpio_t gpio)
{
    status->currtent_degree = DEFAULT_SERVO_MIN_DEGREE;
    status->currtent_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH;
    status->last_pulsewidth = DEFAULT_SERVO_MIN_PLUSEWIDTH;

    status->step_positon = DEFAULT_EASE_STEPS;
    status->is_easing = false;
    status->is_reverse = false;

    HAL_ERR_ASSERT_OK(hal_pwm_open(gpio, SERVO_PWM_FREQUENCY_HZ, SERVO_PWM_RESOLUTION));
}

uint16_t servo_pwmc_pan_per_degree_cal(servo_pwmc_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    degree_of_rotation = degree_of_rotation % MAX_PAN_DEGREE;
    return (config->min_pulsewidth + (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
}

uint16_t servo_pwmc_tilt_per_degree_cal(servo_pwmc_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    uint16_t cal_pulsewidth;
    degree_of_rotation = degree_of_rotation == MAX_TILT_DEGREE ? MAX_TILT_DEGREE : degree_of_rotation % MAX_TILT_DEGREE;
    cal_pulsewidth = (config->min_pulsewidth + (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
    if (is_reverse) {
        cal_pulsewidth = config->max_pulsewidth - (cal_pulsewidth - config->min_pulsewidth);
    }

    return cal_pulsewidth;
}

uint16_t servo_pwmc_ease_cal(ease_config_t *ease_config, servo_pwmc_status_t *status) 
{
    uint16_t easing_pulsewidth = 0;
    
    if (status->last_pulsewidth <= status->currtent_pulsewidth) {
        easing_pulsewidth = status->last_pulsewidth + easeing(ease_config->ease_out, status->step_positon, 0, status->currtent_pulsewidth - status->last_pulsewidth, ease_config->steps);
    } else {
        easing_pulsewidth = status->last_pulsewidth - easeing(ease_config->ease_out, status->step_positon, 0, status->last_pulsewidth - status->currtent_pulsewidth, ease_config->steps);
    }

    if (++status->step_positon >= ease_config->steps) {
        status->is_easing = false;
    }

    return easing_pulsewidth;
}

void servo_pwmc_out(servo_pwmc_status_t *status, uint16_t pulsewidth)
{
    if (status->last_pulsewidth != pulsewidth) {
        uint32_t v = (SERVO_PWM_MAX_VALUE - SERVO_PWM_MIN_VALUE) * (pulsewidth - status->config.min_pulsewidth);
        uint32_t duty = (v / (status->config.max_pulsewidth - status->config.min_pulsewidth)) + SERVO_PWM_MIN_VALUE;
        hal_pwm_set_duty(status->gpio, duty);
        if (!led_mode_is_enable(LED_MODE_TRACKING))
        {
            led_mode_add(LED_MODE_TRACKING);
        }
    }
    status->last_pulsewidth = pulsewidth;
}

void servo_pwmc_update(servo_pwmc_t *servo)
{
    //tilt
    servo_pwmc_control(&servo->internal.tilt, &servo->internal.ease_config);
    //pan
    servo_pwmc_control(&servo->internal.pan, &servo->internal.ease_config);
}

void servo_pwmc_control(servo_pwmc_status_t *status, ease_config_t *ease_config) 
{
    uint16_t out_pulsewidth = 0;

    if (!status->is_easing) {
        status->currtent_pulsewidth = status->pTr_pulsewidth_cal(&status->config, status->currtent_degree, status->is_reverse);
        if (abs(status->last_pulsewidth - status->currtent_pulsewidth) > ease_config->min_pulsewidth) {
            status->is_easing = true;
            status->step_positon = 0;
            out_pulsewidth = servo_pwmc_ease_cal(ease_config, status);
#if defined(USE_BEEPER)
            beeper_set_mode(beeper, BEEPER_MODE_EASING);
#endif
            ESP_LOGI(TAG, "servo easing...");
			led_mode_add(LED_MODE_EASING);
        } else {
            out_pulsewidth = status->currtent_pulsewidth;
        }
    } else {
        out_pulsewidth = servo_pwmc_ease_cal(ease_config, status);
    }

    servo_pwmc_out(status, out_pulsewidth);
}

uint16_t servo_get_degree(servo_pwmc_status_t *status)
{
    return status->currtent_degree;
}

uint32_t servo_get_pulsewidth(servo_pwmc_status_t *status)
{
    return status->currtent_pulsewidth;
}

uint8_t servo_get_per_pulsewidth(servo_pwmc_status_t *status)
{
    return (status->currtent_pulsewidth - status->config.min_pulsewidth) * 100 / (status->config.max_pulsewidth - status->config.min_pulsewidth);
}