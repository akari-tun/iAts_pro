#include <stdint.h>
#include <math.h>

typedef enum ease_out {
	EASE_OUT_QUART = 1,
	EASE_OUT_CIRC = 2,
	EASE_OUT_EXPO = 3,
	EASE_OUT_CUBIC = 4,
} ease_out_t;

typedef struct ease_config_s
{
    uint16_t steps;
    uint16_t min_pulsewidth;
    ease_out_t ease_out;
} ease_config_t;

//Easing functions
float easeing(ease_out_t type, float t, float b, float c, float d);
float easeOutQuart(float t, float b, float c, float d);
float easeOutCirc(float t, float b, float c, float d);
float easeOutExpo(float t, float b, float c, float d);
float easeOutCubic(float t, float b, float c, float d);