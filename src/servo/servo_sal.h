#include <stdint.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "ease_effect.h"

#define MAX_PAN_DEGREE 180
#define MAX_TILT_DEGREE 90

typedef struct servo_mcpwm_config_s
{   uint32_t mcpwm_frequency;
    mcpwm_unit_t mcpwm_unit;
    mcpwm_timer_t mcpwm_timer;
    mcpwm_duty_type_t mcpwm_duty_type;
    mcpwm_counter_type_t mcpwm_counter_mode;
} servo_mcpwm_config_t;

typedef struct servo_config_s
{
    //mcpwm
    mcpwm_io_signals_t mcpwm_io_signals;
    mcpwm_operator_t mcpwm_operator;

    servo_mcpwm_config_t *servo_mcpwm_config;

    //gpio
    uint8_t servo_gpio;

    uint16_t servo_min_pulsewidth;
    uint16_t servo_max_pulsewidth;
    uint16_t servo_max_degree;
    uint16_t servo_min_degree;
} servo_config_t;

typedef struct servo_status_s
{
    uint16_t (*pTr_pulsewidth_cal)(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse);

    uint16_t currtent_degree;
    uint16_t currtent_pulsewidth;
    uint16_t last_pulsewidth;

    uint16_t step_positon;
    bool is_easing;
    bool is_reverse;
} servo_status_t;

uint16_t servo_pan_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse);
uint16_t servo_tilt_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse);
uint16_t servo_ease_cal(ease_effect_config_t *ease_config, servo_status_t *status);
void servo_pwm_out(servo_config_t *config, servo_status_t *status, uint16_t pulsewidth);