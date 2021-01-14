#include "target/target.h"

#if defined(USE_MONITORING)

#include <hal/err.h>
#include <hal/log.h>
#include "config/settings.h"
#include "util/kalman_filter.h"
#include "monitor.h"

#if defined(USE_BATTERY_MONITORING)
static uint32_t vref[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static kalman1_state_t kalman_state;

void battery_init(battery_t *battery)
{
    battery->config.channel = BATTERY_ADC_CHAENNL;
    battery->config.atten = BATTERY_ADC_ATTEN;
    battery->config.unit = BATTERY_ADC_UNIT;
    battery->config.bit_width = BATTERY_ADC_WIDTH;

    HAL_ERR_ASSERT_OK(hal_adc_init(&battery->config));

    battery->vref = (uint32_t *)vref;
    battery->voltage = 0.00f;
    battery->center_voltage = settings_get_key_u16(SETTING_KEY_TRACKER_MONITOR_BATTERY_CENTER_VOLTAGE) / 100.00f;
    battery->max_voltage = settings_get_key_u16(SETTING_KEY_TRACKER_MONITOR_BATTERY_MAX_VOLTAGE) / 100.00f;
    battery->min_voltage = settings_get_key_u16(SETTING_KEY_TRACKER_MONITOR_BATTERY_MIN_VOLTAGE) / 100.00f;
    battery->voltage_scale = settings_get_key_u16(SETTING_KEY_TRACKER_MONITOR_BATTERY_VOLTAGE_SCALE) / 100.00f;
    battery->enable = settings_get_key_bool(SETTING_KEY_TRACKER_MONITOR_BATTERY_ENABLE) ? 1 : 0;

    kalman1_init(&kalman_state, battery->center_voltage, battery->center_voltage);
    kalman_state.q = 4.5;//10e-6;  /* predict noise convariance */
    kalman_state.r = 3.25;//10e-5;  /* measure error convariance */
}

float battery_get_voltage(battery_t *battery)
{   
    // int voltage = kalman1_filter(&kalman_state, get_adc_voltage(&battery->config));
    // uint32_t avg_voltage = 0;
    // uint8_t count = sizeof(vref) / sizeof(vref[0]);

    // for (uint8_t i = count - 1; i >= 1; i--)
    // {
    //     vref[i] = vref[i - 1];
    //     avg_voltage += vref[i];
    // }

    // vref[0] = voltage;
    // avg_voltage = (avg_voltage + voltage) / count;

    // //Calculate the correct voltage according to the partial voltage resistance ratio
    // return avg_voltage / battery->voltage_scale;

    uint32_t voltage = get_adc_voltage(&battery->config);
    uint32_t avg_voltage = 0;
    uint8_t count = sizeof(vref) / sizeof(vref[0]);

    for (uint8_t i = count - 1; i >= 1; i--)
    {
        vref[i] = vref[i - 1];
        avg_voltage += vref[i];
    }

    vref[0] = voltage;
    avg_voltage = (avg_voltage + voltage) / count;

    int filter_voltage =  kalman1_filter(&kalman_state, avg_voltage);

    //Calculate the correct voltage according to the partial voltage resistance ratio
    return filter_voltage / battery->voltage_scale; 
}
#endif

#if defined(USE_POWER_MONITORING)
void power_init(power_t *power)
{
    power->monitoring_gpio = POWER_MONITORING_GPIO;
    power->remote_gpio = POWER_REMOTE_GPIO;
    power->enable = settings_get_key_bool(SETTING_KEY_TRACKER_MONITOR_POWER_ENABLE) ? 1 : 0;
    power->enable_level = settings_get_key_u8(SETTING_KEY_TRACKER_MONITOR_POWER_ENABLE_LEVEL);

    HAL_ERR_ASSERT_OK(hal_gpio_setup(power->monitoring_gpio, HAL_GPIO_DIR_INPUT, HAL_GPIO_PULL_DOWN));
    HAL_ERR_ASSERT_OK(hal_gpio_setup(power->remote_gpio, HAL_GPIO_DIR_OUTPUT, HAL_GPIO_PULL_BOTH));

    HAL_ERR_ASSERT_OK(hal_gpio_set_level(power->remote_gpio, settings_get_key_bool(SETTING_KEY_TRACKER_MONITOR_POWER_TURN) ? HAL_GPIO_LOW : HAL_GPIO_HIGH));

    power->turn_status = 1;
}

uint8_t power_get_power_good(power_t *power)
{
    return hal_gpio_get_level(power->monitoring_gpio);
}

void power_turn_on(power_t *power)
{
    HAL_ERR_ASSERT_OK(hal_gpio_set_level(power->remote_gpio, power->enable_level == 0 ? HAL_GPIO_LOW : HAL_GPIO_HIGH));
    power->turn_status = 1;

}

void power_turn_off(power_t *power)
{
    HAL_ERR_ASSERT_OK(hal_gpio_set_level(power->remote_gpio, power->enable_level == 0 ? HAL_GPIO_HIGH : HAL_GPIO_LOW));
    power->turn_status = 0;
}
#endif

#endif