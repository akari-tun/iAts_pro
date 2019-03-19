
#include <hal/adc.h>

typedef struct battery_s {
    adc_config_t config;
    uint32_t *vref;
} battery_t;

void battery_init(battery_t *battery);
float battery_get_voltage(battery_t *battery);