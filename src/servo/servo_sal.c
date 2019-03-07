#include <stdint.h>
#include "servo_sal.h"

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

uint16_t servo_ease_cal(ease_effect_config_t *ease_config, servo_status_t *status) 
{
    uint16_t easing_pulsewidth = 0;
    
    if (status->last_pulsewidth <= status->currtent_pulsewidth) {
        easing_pulsewidth = status->last_pulsewidth + easeing(ease_config->ease_out_type, status->step_positon, 0, status->currtent_pulsewidth - status->last_pulsewidth, ease_config->steps);
    } else {
        easing_pulsewidth = status->last_pulsewidth - easeing(ease_config->ease_out_type, status->step_positon, 0, status->last_pulsewidth - status->currtent_pulsewidth, ease_config->steps);
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