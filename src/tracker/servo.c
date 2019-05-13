#include <hal/log.h>
#include "servo.h"
#include "observer.h"
#include "config/settings.h"

// static const char *TAG = "servo";

void servo_init(servo_t *servo)
{
    servo_pwm_initialize(servo);

    servo->internal.pan.pTr_pulsewidth_cal = &servo_pan_per_degree_cal;
    servo->internal.tilt.pTr_pulsewidth_cal = &servo_tilt_per_degree_cal;
    servo->internal.reverse_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));

    uint16_t course = settings_get_key_u16(SETTING_KEY_SERVO_COURSE);
    servo->internal.course = course;
}

void servo_update(servo_t *servo)
{
    servo_reverse_check(servo);
    // tilt
    servo_pulsewidth_control(&servo->internal.tilt, &servo->internal.ease_config);
    // pan
    servo_pulsewidth_control(&servo->internal.pan, &servo->internal.ease_config);
}

void servo_pulsewidth_out(servo_status_t *status, uint16_t pulsewidth)
{
    if (status->last_pulsewidth != pulsewidth) {
        servo_pwm_out(status, pulsewidth);
    }
    status->last_pulsewidth = pulsewidth;
}

void servo_reverse_check(servo_t *servo)
{
    bool reverse = servo->internal.pan.currtent_degree > 180;
    if (servo->is_reversing != reverse)
    {
        // servo->internal.tilt.is_reverse = reverse;
        servo->internal.pan.is_reverse = reverse;
        servo->is_reversing = reverse;
        servo->internal.reverse_notifier->mSubject.Notify(servo->internal.reverse_notifier, &reverse);
    }
}

void servo_pulsewidth_control(servo_status_t *status, ease_config_t *ease_config)
{
    uint16_t out_pulsewidth = 0;

    if (!status->is_easing || status->step_positon >= status->step_to) {
        status->currtent_pulsewidth = status->pTr_pulsewidth_cal(&status->config, status->currtent_degree, status->is_reverse);
        uint32_t pwm_error = abs(status->last_pulsewidth - status->currtent_pulsewidth);
        if (pwm_error > ease_config->min_pulsewidth) {
            status->is_easing = true;
            status->step_positon = 0;
            status->step_to = pwm_error > DEFAULT_EASE_MAX_STEPS ? DEFAULT_EASE_MAX_STEPS : pwm_error;
            status->step_sleep_ms = DEFAULT_EASE_STEP_MS;
            if (status->step_sleep_ms * status->step_to > ease_config->max_ms) status->step_sleep_ms = ease_config->max_ms / status->step_to;
            if (status->step_sleep_ms * status->step_to < ease_config->min_ms) status->step_sleep_ms = ease_config->min_ms / status->step_to;
            out_pulsewidth = servo_ease_cal(ease_config, status);
        } else {
            out_pulsewidth = status->currtent_pulsewidth;
        }
    } else {
        out_pulsewidth = servo_ease_cal(ease_config, status);
    }

    servo_pulsewidth_out(status, out_pulsewidth);
}

uint16_t servo_pan_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    degree_of_rotation = degree_of_rotation % MAX_PAN_DEGREE;
    // return (config->min_pulsewidth + (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
    return (config->max_pulsewidth - (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
}

uint16_t servo_tilt_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    uint16_t cal_pulsewidth;
    degree_of_rotation = degree_of_rotation == MAX_TILT_DEGREE ? MAX_TILT_DEGREE : degree_of_rotation % MAX_TILT_DEGREE;
    cal_pulsewidth = (config->min_pulsewidth + (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
    if (!is_reverse) {
        cal_pulsewidth = config->max_pulsewidth - (cal_pulsewidth - config->min_pulsewidth);
    }

    return cal_pulsewidth;
}

uint16_t servo_ease_cal(ease_config_t *ease_config, servo_status_t *status)
{
    uint16_t easing_pulsewidth = 0;
    
    if (status->last_pulsewidth <= status->currtent_pulsewidth) {
        easing_pulsewidth = status->last_pulsewidth + easeing(ease_config->ease_out, status->step_positon, 0, status->currtent_pulsewidth - status->last_pulsewidth, status->step_to);
    } else {
        easing_pulsewidth = status->last_pulsewidth - easeing(ease_config->ease_out, status->step_positon, 0, status->last_pulsewidth - status->currtent_pulsewidth, status->step_to);
    }

    if (++status->step_positon >= status->step_to) {
        status->is_easing = false;
    }

    return easing_pulsewidth;
}

uint16_t servo_get_degree(servo_status_t *status)
{
    return status->currtent_degree;
}

uint16_t servo_get_course_to_degree(servo_t *servo)
{
    uint16_t deg = servo->internal.pan.currtent_degree < servo->internal.course ? servo->internal.pan.currtent_degree + 360u : servo->internal.pan.currtent_degree;
    return deg - servo->internal.course;
}

uint32_t servo_get_pulsewidth(servo_status_t *status)
{
    return status->last_pulsewidth;
}

uint32_t servo_get_easing_sleep(servo_status_t *status)
{
    return status->step_sleep_ms;
}

uint8_t servo_get_per_pulsewidth(servo_status_t *status)
{
    return (status->last_pulsewidth - status->config.min_pulsewidth) * 100 / (status->config.max_pulsewidth - status->config.min_pulsewidth);
}