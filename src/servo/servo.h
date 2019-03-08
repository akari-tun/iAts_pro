
#include "soc/mcpwm_struct.h"

#include "servo_sal.h"

#define SERVO_MCPWM_FREQUENCY 50

#define SERVO_MCPWM_UNIT MCPWM_UNIT_0
#define SERVO_MCPWM_TIMER MCPWM_TIMER_0
#define SERVO_MCPWM_DUTY_MODE MCPWM_DUTY_MODE_0
#define SERVO_MCPWM_COUNTER_MODE MCPWM_UP_COUNTER

#define SERVO_PAN_MCPWM_IO_SIGNALS MCPWM0A
#define SERVO_TILT_MCPWM_IO_SIGNALS MCPWM0B

#define SERVO_PAN_MCPWM_OPR MCPWM_OPR_A
#define SERVO_TILT_MCPWM_OPR MCPWM_OPR_B

#define SERVO_TILT_MIN_ANGLE 0
#define SERVO_TILT_MAX_ANGLE 90
#define SERVO_PAN_MIN_ANGLE 0
#define SERVO_PAN_MAX_ANGLE 359

typedef struct servo_s
{
    servo_mcpwm_config_t servo_mcpwm_config;

    servo_config_t servo_pan_config;
    servo_config_t servo_tilt_config;
    ease_config_t servo_ease_config;

    servo_status_t servo_pan_status;
    servo_status_t servo_tilt_status;
} servo_t;

void servo_gpio_initialize(servo_t *servo);
void servo_configuration(servo_t *servo);
void servo_update(servo_t *servo);
void servo_control(servo_config_t *servo_config, ease_config_t *ease_config, servo_status_t *status);
