#include <stdint.h>
#include <math.h>

typedef enum ease_out {
	EASE_OUT_QUAD = 0,
	EASE_OUT_QUART,
	EASE_OUT_CIRC,
	EASE_OUT_EXPO,
	EASE_OUT_CUBIC,
} ease_out_t;

typedef struct ease_config_s
{
    uint16_t max_steps;
	uint16_t max_ms;
	uint16_t min_ms;
    uint16_t min_pulsewidth;
	uint16_t step_ms;
    ease_out_t ease_out;
} ease_config_t;

//Easing functions
float easeing(ease_out_t type, float t, float b, float c, float d);
float easeOutQuad(float t, float b, float c, float d);
float easeOutQuart(float t, float b, float c, float d);
float easeOutCirc(float t, float b, float c, float d);
float easeOutExpo(float t, float b, float c, float d);
float easeOutCubic(float t, float b, float c, float d);