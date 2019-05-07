#include <hal/log.h>
#include "servo.h"
#include "observer.h"

// #include "ui/led.h"
// #include "ui/beeper.h"

// #if defined(USE_BEEPER)
//     static beeper_t *beeper;
// #endif

// static const char *TAG = "servo";

// void servo_init(servo_t *servo, ui_t *ui)
void servo_init(servo_t *servo)
{
    servo_pwm_initialize(servo);

// #if defined(USE_BEEPER)
//     beeper = &ui->internal.beeper;
// #endif

    servo->internal.pan.pTr_pulsewidth_cal = &servo_pan_per_degree_cal;
    servo->internal.tilt.pTr_pulsewidth_cal = &servo_tilt_per_degree_cal;
    servo->internal.reverse_notifier = (notifier_t *)Notifier_Create(sizeof(notifier_t));
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
// #if defined(USE_TRACKING_LED) 
//         if (!led_mode_is_enable(LED_MODE_TRACKING))
//         {
//             led_mode_add(LED_MODE_TRACKING);
//         }
// #endif
    }
    status->last_pulsewidth = pulsewidth;
}

void servo_reverse_check(servo_t *servo)
{
    bool reverse = servo->internal.pan.currtent_degree > 180;
    if (servo->is_reversing != reverse)
    {
        servo->internal.tilt.is_reverse = reverse;
        servo->internal.pan.is_reverse = reverse;
        servo->is_reversing = reverse;
        servo->internal.reverse_notifier->mSubject.Notify(servo->internal.reverse_notifier, &reverse);
    }
}

void servo_pulsewidth_control(servo_status_t *status, ease_config_t *ease_config)
{
    uint16_t out_pulsewidth = 0;

    if (!status->is_easing) {
        status->currtent_pulsewidth = status->pTr_pulsewidth_cal(&status->config, status->currtent_degree, status->is_reverse);
        if (abs(status->last_pulsewidth - status->currtent_pulsewidth) > ease_config->min_pulsewidth) {
            status->is_easing = true;
            status->step_positon = 0;
            out_pulsewidth = servo_ease_cal(ease_config, status);
// #if defined(USE_BEEPER)
//             beeper_set_mode(beeper, BEEPER_MODE_EASING);
// #endif
//             LOG_I(TAG, "servo easing...");
// 			led_mode_add(LED_MODE_EASING);
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
    return (config->min_pulsewidth + (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
}

uint16_t servo_tilt_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse)
{
    uint16_t cal_pulsewidth;
    degree_of_rotation = degree_of_rotation == MAX_TILT_DEGREE ? MAX_TILT_DEGREE : degree_of_rotation % MAX_TILT_DEGREE;
    cal_pulsewidth = (config->min_pulsewidth + (((config->max_pulsewidth - config->min_pulsewidth) * (degree_of_rotation)) / (config->max_degree)));
    if (is_reverse) {
        cal_pulsewidth = config->max_pulsewidth - (cal_pulsewidth - config->min_pulsewidth);
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

uint16_t servo_get_degree(servo_status_t *status)
{
    return status->currtent_degree;
}

uint32_t servo_get_pulsewidth(servo_status_t *status)
{
    return status->currtent_pulsewidth;
}

uint8_t servo_get_per_pulsewidth(servo_status_t *status)
{
    return (status->currtent_pulsewidth - status->config.min_pulsewidth) * 100 / (status->config.max_pulsewidth - status->config.min_pulsewidth);
}