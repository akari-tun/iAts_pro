#include <hal/err.h>
#include <hal/log.h>
#include "target/target.h"
#include "battery.h"
#include "util/kalman_filter.h"

static uint32_t vref[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static kalman1_state_t kalman_state;

void battery_init(battery_t *battery)
{
    battery->config.channel = BATTERY_ADC_CHAENNL;
    battery->config.atten = BATTERY_ADC_ATTEN;
    battery->config.unit = BATTERY_ADC_UNIT;
    battery->config.bit_width = BATTERY_ADC_WIDTH;

    HAL_ERR_ASSERT_OK(hal_adc_init(&battery->config));

    battery->vref = (uint32_t *)vref;

    kalman1_init(&kalman_state, 0, 0);
}

float battery_get_voltage(battery_t *battery)
{   
    uint32_t voltage = get_adc_voltage(&battery->config);
    uint32_t avg_voltage = 0;

    for (uint8_t i = 9; i >= 1; i--)
    {
        vref[i] = vref[i - 1];
        avg_voltage += vref[i];
    }

    vref[0] = voltage;
    avg_voltage = (avg_voltage + voltage) / 10;

    //Calculate the correct voltage according to the partial voltage resistance ratio
    return kalman1_filter(&kalman_state, (avg_voltage / BATTEYR_PARTIAL_PRESSURE_VALUE)); 
}