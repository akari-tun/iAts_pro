#include <hal/gpio.h>
#include "default.h"

#define USE_SCREEN
#define SCREEN_I2C_BUS I2C_NUM_1
#define SCREEN_I2C_ADDR 0x3c
#define SCREEN_GPIO_SDA 32
#define SCREEN_GPIO_SCL 33
#define SCREEN_GPIO_RST 0

#define USE_BEEPER
#define BEEPER_GPIO 13

#define USE_BUTTON_5WAY
#define BUTTON_ENTER_GPIO 0
#define BUTTON_RIGHT_GPIO 34
#define BUTTON_LEFT_GPIO 12
#define BUTTON_UP_GPIO 14
#define BUTTON_DOWN_GPIO 35

#define USE_LED
#define LED_USE_WS2812
#define LED_USE_FADING
#define LED_1_GPIO 21
#define LED_1_USE_WS2812

#define SERVO_PAN_GPIO 27
#define SERVO_TILT_GPIO 26
