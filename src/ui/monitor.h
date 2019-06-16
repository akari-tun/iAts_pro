#pragma once

#if defined(USE_MONITORING)

#include <hal/adc.h>
#include <hal/gpio.h>

#if defined(USE_BATTERY_MONITORING)
typedef struct battery_s {
    adc_config_t config;
    uint32_t *vref;
    float voltage_scale;
    float max_voltage;
    float min_voltage;
    float center_voltage;
    uint8_t enable;
} battery_t;

void battery_init(battery_t *battery);
float battery_get_voltage(battery_t *battery);
#endif

#if defined(USE_POWER_MONITORING)
typedef struct power_s {
    uint8_t monitoring_gpio;
    uint8_t remote_gpio;

    uint8_t turn_status;
    uint8_t enable;
} power_t;

void power_init(power_t *power);

uint8_t power_get_power_good(power_t *power);
void power_turn_on(power_t *power);
void power_turn_off(power_t *power);
#endif

#endif