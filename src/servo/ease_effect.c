/*
 * The code bellow uses Easing Ecuations by Robert Penner http://www.gizma.com/easing/
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "ease_effect.h"

float easeing(ease_effect_out_typt_t type, float t, float b, float c, float d) {
    switch (type)
    {
        case EASE_OUT_QRT:
            return easeOutQuart(t, b, c, d);
        case EASE_OUT_CIRC:
            return easeOutCirc(t, b, c, d);
        case EASE_OUT_EXPO:
            return easeOutExpo(t, b, c, d);
        case EASE_OUT_CUBIC:
            return easeOutCubic(t, b, c, d);
        default:
            return easeOutQuart(t, b, c, d);
    }
}

float easeOutQuart(float t, float b, float c, float d) {
	t /= d/2;
	if (t < 1) return c/2*t*t + b;
	t--;
	return -c/2 * (t*(t-2) - 1) + b;
}

float easeOutCirc(float t, float b, float c, float d) {
	t /= d/2;
	if (t < 1) return -c/2 * (sqrt(1 - t*t) - 1) + b;
	t -= 2;
	return c/2 * (sqrt(1 - t*t) + 1) + b;
}

float easeOutExpo(float t, float b, float c, float d) {
	return (t==d) ? b+c : c * (-pow(2, -10 * t/d) + 1) + b;
}

float easeOutCubic(float t, float b, float c, float d) {
	t /= d;
	t--;
	return c*(t*t*t + 1) + b;
}

