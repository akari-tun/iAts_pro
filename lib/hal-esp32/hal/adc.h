#include <esp_adc_cal.h>

typedef struct adc_config_s {
    uint8_t channel;
    uint8_t atten;
    uint8_t unit;
    uint8_t bit_width;
    esp_adc_cal_characteristics_t adc_chars;
} adc_config_t;

#include <hal/adc_base.h>
