#include <esp_log.h>
#include "servo.h"

static const char *TAG = "servo";

void servo_gpio_initialize(servo_t *servo)
{
    ESP_LOGI(TAG, "servo_gpio_initialize");
    mcpwm_gpio_init(servo->servo_mcpwm_config.mcpwm_unit, servo->servo_pan_config.mcpwm_io_signals, servo->servo_pan_config.servo_gpio);
	mcpwm_gpio_init(servo->servo_mcpwm_config.mcpwm_unit, servo->servo_tilt_config.mcpwm_io_signals, servo->servo_tilt_config.servo_gpio);    
}

void servo_configuration(servo_t *servo)
{
    ESP_LOGI(TAG, "servo_configuration");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = servo->servo_mcpwm_config.mcpwm_frequency;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 50;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 50;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = servo->servo_mcpwm_config.mcpwm_counter_mode;
    pwm_config.duty_mode = servo->servo_mcpwm_config.mcpwm_duty_type;
    mcpwm_init(servo->servo_mcpwm_config.mcpwm_unit, servo->servo_mcpwm_config.mcpwm_timer, &pwm_config);    //Configure PWM0A & PWM0B with above settings  

    servo->servo_pan_status.pTr_pulsewidth_cal = &servo_pan_per_degree_cal;
    servo->servo_tilt_status.pTr_pulsewidth_cal = &servo_tilt_per_degree_cal;
}

void servo_update(servo_t *servo)
{
    servo->servo_tilt_status.is_reverse = servo->servo_pan_status.currtent_degree > 180;
    servo_control(&servo->servo_pan_config, &servo->servo_ease_config, &servo->servo_pan_status);
    servo_control(&servo->servo_tilt_config, &servo->servo_ease_config, &servo->servo_tilt_status);
}

void servo_control(servo_config_t *servo_config, ease_config_t *ease_config, servo_status_t *status) 
{
    uint16_t out_pulsewidth = 0;
    if (!status->is_easing) {
        status->currtent_pulsewidth = status->pTr_pulsewidth_cal(servo_config, status->currtent_degree, status->is_reverse);
        if (abs(status->last_pulsewidth - status->currtent_pulsewidth) > ease_config->min_pulsewidth) {
            status->is_easing = true;
            status->step_positon = 0;
            out_pulsewidth = servo_ease_cal(ease_config, status);
        } else {
            out_pulsewidth = status->currtent_pulsewidth;
        }
    } else {
        out_pulsewidth = servo_ease_cal(ease_config, status);
    }

    servo_pwm_out(servo_config, status, out_pulsewidth);
}


uint16_t servo_pan_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    degree_of_rotation = degree_of_rotation % MAX_PAN_DEGREE;
    return (config->servo_min_pulsewidth + (((config->servo_max_pulsewidth - config->servo_min_pulsewidth) * (degree_of_rotation)) / (config->servo_max_degree)));
}

uint16_t servo_tilt_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    uint16_t cal_pulsewidth;
    degree_of_rotation = degree_of_rotation == MAX_TILT_DEGREE ? MAX_TILT_DEGREE : degree_of_rotation % MAX_TILT_DEGREE;
    cal_pulsewidth = (config->servo_min_pulsewidth + (((config->servo_max_pulsewidth - config->servo_min_pulsewidth) * (degree_of_rotation)) / (config->servo_max_degree)));
    if (is_reverse) {
        cal_pulsewidth = config->servo_max_pulsewidth - (cal_pulsewidth - config->servo_min_pulsewidth);
    }

    return cal_pulsewidth;
}

uint16_t servo_ease_cal(ease_config_t *ease_config, servo_status_t *status) 
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

void servo_pwm_out(servo_config_t *config, servo_status_t *status, uint16_t pulsewidth)
{
    if (status->last_pulsewidth != pulsewidth) {
        mcpwm_set_duty_in_us(config->servo_mcpwm_config->mcpwm_unit, config->servo_mcpwm_config->mcpwm_timer, config->mcpwm_operator, pulsewidth);
    }
    status->last_pulsewidth = pulsewidth;
}