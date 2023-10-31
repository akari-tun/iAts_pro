#include <stdint.h>

#include <hal/err.h>
#include <hal/gpio.h>

hal_err_t hal_adc_init(adc_config_t *config);
uint32_t get_adc_voltage(adc_config_t *config);