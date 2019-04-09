#include <stdint.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "target/target.h"
#include "ui/ui.h"

#if defined(ESP32) && defined(USE_MCPWM)
#include "servo_mcpwm.h"
typedef struct servo_mcpwm_s servo_t;
typedef struct servo_mcpwm_config_s servo_config_t;
typedef struct servo_mcpwm_status_s servo_status_t;
#else
#include "servo_pwmc.h"
typedef struct servo_pwmc_s servo_t;
typedef struct servo_pwmc_config_s servo_config_t;
typedef struct servo_pwmc_status_s servo_status_t;
#endif

void servo_init(servo_t *servo_pwmc, ui_t *ui);
void servo_update(servo_t *servo_pwmc);
void servo_pulsewidth_out(servo_status_t *status, uint16_t pulsewidth);
void servo_pulsewidth_control(servo_status_t *status, ease_config_t *ease_config);
uint16_t servo_pan_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse);
uint16_t servo_tilt_per_degree_cal(servo_config_t *config, uint16_t degree_of_rotation, bool is_reverse);
uint16_t servo_ease_cal(ease_config_t *ease_config, servo_status_t *status);
uint16_t servo_get_degree(servo_status_t *status);
uint32_t servo_get_pulsewidth(servo_status_t *status);
uint8_t servo_get_per_pulsewidth(servo_status_t *status);