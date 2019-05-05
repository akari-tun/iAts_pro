#include "calc.h"
#include <math.h>

float distance_between(float lat1, float long1, float lat2, float long2)
{
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothetical sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be up to 0.5%.
	// Courtesy of Maarten Lamers
	float delta = radians(long1 - long2);
	float sdlong = sin(delta);
	float cdlong = cos(delta);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	float slat1 = sin(lat1);
	float clat1 = cos(lat1);
	float slat2 = sin(lat2);
	float clat2 = cos(lat2);
	delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
	delta = sq(delta);
	delta += sq(clat2 * sdlong);
	delta = sqrt(delta);
	float denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
	delta = atan2(delta, denom);
	return delta * 6372795;
}

float course_to(float lat1, float long1, float lat2, float long2)
{
	// returns course in degrees (North=0, West=270) from position 1 to position 2,
	// both specified as signed decimal-degrees latitude and longitude.
	// Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
	// Courtesy of Maarten Lamers
	float dlon = radians(long2 - long1);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	float a1 = sin(dlon) * cos(lat2);
	float a2 = sin(lat1) * cos(lat2) * cos(dlon);
	a2 = cos(lat1) * sin(lat2) - a2;
	a2 = atan2(a1, a2);
	if (a2 < 0.0)
	{
		a2 += TWO_PI;
	}
	return degrees(a2);
}

uint16_t tilt_to(uint16_t distance, uint32_t alt1, uint32_t alt2)
{
    int16_t alpha = 0;

	//this will fix an error where the tracker randomly points up when plane is lower than tracker
	if (alt2 < alt1) alt2 = alt1;
	//prevent division by 0
    // in larger altitude 1m distance shouldnt mess up the calculation.
	//e.g. in 100m height and dist 1 the angle is 89.4° which is actually accurate enough
	if (distance == 0) distance = 1;

	alpha = toDeg(atan((alt2 / 100.00f - alt1 / 100.00f) / distance));

	//just for current tests, later we will have negative tilt as well
	if (alpha < 0) alpha = 0;
	else if (alpha > 90) alpha = 90;

    return alpha;
}