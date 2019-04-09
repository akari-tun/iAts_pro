#include "target/target.h"

#if defined(USE_PWMC)
#include "ease.h"

#define SERVO_PWM_FREQUENCY_HZ 50

/*
    use 15bit resolution from max value is 32767.
    at 50hz frequency is 20ms(20000us) and map to pwm value 500us ~ 2500us is (32767 * (500 / 20000)) ~ (32767 * (2500 / 20000)).
*/
#define SERVO_PWM_RESOLUTION 15
#define SERVO_PWM_DUTY_MAX_VALUE ((1 << SERVO_PWM_RESOLUTION) - 1)
#define SERVO_PWM_MIN_VALUE (SERVO_PWM_DUTY_MAX_VALUE * (DEFAULT_SERVO_MIN_PLUSEWIDTH / 20000.00f))
#define SERVO_PWM_MAX_VALUE (SERVO_PWM_DUTY_MAX_VALUE * (DEFAULT_SERVO_MAX_PLUSEWIDTH / 20000.00f))

#define SERVO_PWM_CHANNEL_RANGE (DEFAULT_SERVO_MAX_PLUSEWIDTH - DEFAULT_SERVO_MIN_PLUSEWIDTH)

#define MAX_PAN_DEGREE 180
#define MAX_TILT_DEGREE 90

typedef struct servo_pwmc_config_s
{
    //gpio
    uint8_t gpio;

    uint16_t min_pulsewidth;
    uint16_t max_pulsewidth;
    uint16_t max_degree;
    uint16_t min_degree;
} servo_pwmc_config_t;

typedef struct servo_pwmc_status_s
{
    uint16_t currtent_degree;
    uint32_t currtent_pulsewidth;
    uint32_t last_pulsewidth;

    uint16_t step_positon;
    bool is_easing;
    bool is_reverse;

    servo_pwmc_config_t config;
    uint16_t (*pTr_pulsewidth_cal)(servo_pwmc_config_t *config, uint16_t degree_of_rotation, bool is_reverse);
} servo_pwmc_status_t;

typedef struct servo_pwmc_s
{
    struct 
    { 
        servo_pwmc_status_t pan;
        servo_pwmc_status_t tilt;

        ease_config_t ease_config;
    } internal;
} servo_pwmc_t;

void servo_pwm_initialize(servo_pwmc_t *servo_pwmc);
void servo_pwm_configuration(servo_pwmc_status_t *status, hal_gpio_t gpio);
void servo_pwm_out(servo_pwmc_status_t *status, uint16_t pulsewidth);
#endif