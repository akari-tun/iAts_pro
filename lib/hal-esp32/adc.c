#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <hal/adc.h>
#include "hal/log.h"
#include "hal/gpio.h"

#define NO_OF_SAMPLES 10           //Multisampling
#define DEFAULT_VREF  1100        //Use adc2_vref_to_gpio() to obtain a better estimate

static const char *TAG = "ESP_ADC";

hal_err_t hal_adc_init(adc_config_t *config)
{
    esp_err_t err;

    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        LOG_I(TAG, "eFuse Two Point: Supported");
    } else {
        LOG_I(TAG, "eFuse Two Point: NOT supported");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        LOG_I(TAG, "eFuse Vref: Supported");
    } else {
        LOG_I(TAG, "eFuse Vref: NOT supported");
    }

    if ((adc_unit_t)config->unit == ADC_UNIT_1) {
        if ((err = adc1_config_width((adc_bits_width_t)config->bit_width)) != ESP_OK) {
            return err;
        }  

        if ((err = adc1_config_channel_atten((adc_channel_t)config->channel, (adc_atten_t)config->atten)) != ESP_OK) {
            return err;
        }    
    } else {
        if ((err = adc2_config_channel_atten((adc2_channel_t)config->channel, (adc_atten_t)config->atten)) != ESP_OK) {
            return err;
        }
    }

    //Characterize ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)config->unit, 
        (adc_atten_t)config->atten, 
        (adc_bits_width_t)config->bit_width, 
        DEFAULT_VREF, 
        &config->adc_chars);

    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        LOG_I(TAG, "Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        LOG_I(TAG, "Characterized using eFuse Vref");
    } else {
        LOG_I(TAG, "Characterized using Default Vref");
    }

    return err;
}

uint32_t get_adc_voltage(adc_config_t *config)
{
    uint32_t adc_reading = 0;

    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (config->unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)config->channel);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)config->channel, (adc_bits_width_t)config->bit_width, &raw);
            adc_reading += raw;
        }
    }

    adc_reading /= NO_OF_SAMPLES;

    return esp_adc_cal_raw_to_voltage(adc_reading, &config->adc_chars);
}