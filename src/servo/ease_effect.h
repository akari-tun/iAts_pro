#include <stdint.h>
#include <math.h>

typedef enum ease_effect_out_typt {
	EASE_OUT_QRT = 1,
	EASE_OUT_CIRC = 2,
	EASE_OUT_EXPO = 3,
	EASE_OUT_CUBIC = 4,
} ease_effect_out_typt_t;

typedef struct ease_effect_config_s
{
    uint16_t steps;
    uint16_t min_pulsewidth;
    ease_effect_out_typt_t ease_out_type;
} ease_effect_config_t;

//Easing functions
float easeing(ease_effect_out_typt_t type, float t, float b, float c, float d);
float easeOutQuart(float t, float b, float c, float d);
float easeOutCirc(float t, float b, float c, float d);
float easeOutExpo(float t, float b, float c, float d);
float easeOutCubic(float t, float b, float c, float d);