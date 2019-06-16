#pragma once

#include <hal/gpio.h>
#include <driver/adc.h>
#include <driver/i2c.h>
#include "default.h"

#define USE_MCPWM
//#define USE_PWMC
//#define USE_TRACKING_LED

#define USE_SCREEN
#define SCREEN_I2C_BUS I2C_NUM_1
#define SCREEN_I2C_ADDR 0x3c
#define SCREEN_GPIO_SDA 32
#define SCREEN_GPIO_SCL 33
#define SCREEN_GPIO_RST HAL_GPIO_NONE

#define USE_BEEPER
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

#define TX_DEFAULT_GPIO 27
#define RX_DEFAULT_GPIO 26

#define TX_UNUSED_GPIO 26
#define RX_UNUSED_GPIO 27

#define HAL_GPIO_USER_MASK (HAL_GPIO_M(TX_DEFAULT_GPIO) | HAL_GPIO_M(RX_DEFAULT_GPIO) | HAL_GPIO_M(TX_UNUSED_GPIO) | HAL_GPIO_M(RX_UNUSED_GPIO))

#define USE_MONITORING

#if defined(USE_MONITORING)
    #define USE_BATTERY_MONITORING
    /* The ratio of voltage resistors */
    #define BATTERY_PARTIAL_PRESSURE_VALUE 90.00f
    #define BATTERY_ADC_CHAENNL ADC1_CHANNEL_0
    #define BATTERY_ADC_ATTEN ADC_ATTEN_DB_2_5
    #define BATTERY_ADC_UNIT ADC_UNIT_1
    #define BATTERY_ADC_WIDTH ADC_WIDTH_BIT_12

    #define USE_POWER_MONITORING
    #define POWER_MONITORING_GPIO 2
    #define POWER_REMOTE_GPIO 15
#endif