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

void distance_move_to(float beginLat, float beginLon, float orient, float distance, float *distLat, float *distLon)
{
	distance = distance * 1852;

	if (fabs(orient - 90) <= 0.0001 || fabs(orient - 270) <= 0.0001)
	{
		*distLat = beginLat;
		float lonDist = (pow(1 - e_Earth * e_Earth * pow(sin(radians(beginLat)), 2), 0.5) * distance) / (A_Earth * cos(radians(beginLat)));

		if (fabs(orient - 90) <= 0.0000001)
		{
			*distLon = fabs(orient - 90) <= 0.0000001 ? degrees(radians(beginLon) + lonDist) : degrees(radians(beginLon) - lonDist);
		}
	}
	else
	{
		float A1 = 1+ 3. / 4 * pow(e_Earth, 2) + 45. / 64 * pow(e_Earth, 4) + 175. / 256 * pow(e_Earth, 6) + 11025. / 16384 * pow(e_Earth,8);
		float A2 = -3. / 8 * pow(e_Earth, 2) - 15. / 32 * pow(e_Earth, 4) - 525. / 1024 * pow(e_Earth, 6) - 2205. / 4096 * pow(e_Earth,8);
		float A3 = 15. / 256 * pow(e_Earth, 4) + 105. / 1024 * pow(e_Earth, 6) + 2205. / 16384 * pow(e_Earth, 8);
		float A4 = -35. / 3072 * pow(e_Earth, 6) - 105. / 4096 * pow(e_Earth, 8);
		float A5 = 315. / 131072 * pow(e_Earth,8);
		float B1 = 3. / 8 * pow(e_Earth, 2) + 3. / 16 * pow(e_Earth, 4) + 213. / 2048 * pow(e_Earth, 6) + 255. / 4096 * pow(e_Earth, 8);
		float B2 = 21. / 256 * pow(e_Earth, 4) + 21. / 256 * pow(e_Earth, 6) + 533. / 8192 * pow(e_Earth,8);
		float B3 = 151. / 6144 * pow(e_Earth, 6) + 151. / 4096 * pow(e_Earth, 8);
		float B4 = 1097. / 131082 * pow(e_Earth, 8);

		float x1 = A_Earth * (1 - e_Earth * e_Earth) * (A1 * radians(beginLat) + A2 * sin(radians(2 * beginLat)) + A3 * sin(radians( 4 * beginLat)) + A4 * sin(radians(6 * beginLat)) + A5 * sin(radians(8 * beginLat)));
		float x2 = x1 + distance * cos(radians(orient));
		float b0 = x2 / ((1 - e_Earth * e_Earth) * A_Earth * A1);

		*distLat = b0 + B1 * sin(2 * b0) + B2 * sin(4 * b0) + B3 * sin(6 * b0) + B4 * sin(8 * b0);
		*distLat = degrees(*distLat);

		float sin1 = sin(radians(beginLat));
		float sin2 = sin(radians(*distLat));
		float q1 = log((1 + sin1) / (1 - sin1)) / 2 - e_Earth * log((1 + sin1 * e_Earth) / (1 - sin1 * e_Earth)) / 2;
		float q2 = log((1 + sin2) / (1 - sin2)) / 2 - e_Earth * log((1 + sin2 * e_Earth) / (1 - sin2 * e_Earth)) / 2;
		*distLon = radians(beginLon) + (q2 - q1) * tan(radians(orient));
		*distLon = degrees(*distLon);
	}
}