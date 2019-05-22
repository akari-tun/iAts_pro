
#include <hal/adc.h>

typedef struct battery_s {
    adc_config_t config;
    uint32_t *vref;
    float voltage_scale;
    float max_voltage;
    float min_voltage;
    float center_voltage;
} battery_t;

void battery_init(battery_t *battery);
float battery_get_voltage(battery_t *battery);