#include <hal/gpio.h>
#include "default.h"

#define USE_SCREEN
#define SCREEN_I2C_BUS I2C_NUM_1
#define SCREEN_I2C_ADDR 0x3c
#define SCREEN_GPIO_SDA 32
#define SCREEN_GPIO_SCL 33
#define SCREEN_GPIO_RST 0

//#define USE_BEEPER
#define BEEPER_GPIO 13

#define USE_BUTTON_5WAY
#define BUTTON_ENTER_GPIO 0
#define BUTTON_RIGHT_GPIO 34
#define BUTTON_LEFT_GPIO 14
#define BUTTON_UP_GPIO 35
#define BUTTON_DOWN_GPIO 12

#define USE_LED
#define LED_USE_WS2812
#define LED_USE_FADING
#define LED_1_GPIO 21
#define LED_1_USE_WS2812

#define USE_WIFI

#define SERVO_PAN_GPIO 27
#define SERVO_TILT_GPIO 26

#define USE_BATTERY_MEASUREMENT
/* The ratio of partial voltage resistors */
#define BATTEYR_PARTIAL_PRESSURE_VALUE 89.48f
/*
    ADC1_CHANNEL_0 = 0, !< ADC1 channel 0 is GPIO36 
    ADC1_CHANNEL_1,     !< ADC1 channel 1 is GPIO37 
    ADC1_CHANNEL_2,     !< ADC1 channel 2 is GPIO38 
    ADC1_CHANNEL_3,     !< ADC1 channel 3 is GPIO39 
    ADC1_CHANNEL_4,     !< ADC1 channel 4 is GPIO32 
    ADC1_CHANNEL_5,     !< ADC1 channel 5 is GPIO33 
    ADC1_CHANNEL_6,     !< ADC1 channel 6 is GPIO34 
    ADC1_CHANNEL_7,     !< ADC1 channel 7 is GPIO35 

    ADC2_CHANNEL_0 = 0, !< ADC2 channel 0 is GPIO4 
    ADC2_CHANNEL_1,     !< ADC2 channel 1 is GPIO0 
    ADC2_CHANNEL_2,     !< ADC2 channel 2 is GPIO2 
    ADC2_CHANNEL_3,     !< ADC2 channel 3 is GPIO15 
    ADC2_CHANNEL_4,     !< ADC2 channel 4 is GPIO13 
    ADC2_CHANNEL_5,     !< ADC2 channel 5 is GPIO12 
    ADC2_CHANNEL_6,     !< ADC2 channel 6 is GPIO14 
    ADC2_CHANNEL_7,     !< ADC2 channel 7 is GPIO27 
    ADC2_CHANNEL_8,     !< ADC2 channel 8 is GPIO25 
    ADC2_CHANNEL_9,     !< ADC2 channel 9 is GPIO26 
    ADC2_CHANNEL_MAX,
*/
#define BATTERY_ADC_CHAENNL 0
/*
    ADC_ATTEN_DB_0   = 0,  !<The input voltage of ADC will be reduced to about 1/1 
    ADC_ATTEN_DB_2_5 = 1,  !<The input voltage of ADC will be reduced to about 1/1.34 
    ADC_ATTEN_DB_6   = 2,  !<The input voltage of ADC will be reduced to about 1/2 
    ADC_ATTEN_DB_11  = 3,  !<The input voltage of ADC will be reduced to about 1/3.6
*/
#define BATTERY_ADC_ATTEN 1
/*
    ADC_UNIT_1 = 1,          !< SAR ADC 1
    ADC_UNIT_2 = 2,          !< SAR ADC 2, not supported yet
    ADC_UNIT_BOTH = 3,       !< SAR ADC 1 and 2, not supported yet 
    ADC_UNIT_ALTER = 7,      !< SAR ADC 1 and 2 alternative mode, not supported yet 
    ADC_UNIT_MAX,
*/
#define BATTERY_ADC_UNIT 1
/*
 *   ADC_WIDTH_BIT_9  = 0, !< ADC capture width is 9Bit
 *   ADC_WIDTH_BIT_10 = 1, !< ADC capture width is 10Bit
 *   ADC_WIDTH_BIT_11 = 2, !< ADC capture width is 11Bit
 *   ADC_WIDTH_BIT_12 = 3, !< ADC capture width is 12Bit
 *   ADC_WIDTH_MAX
*/
#define BATTERY_ADC_WIDTH 3