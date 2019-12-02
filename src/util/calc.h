#include <stddef.h>
#include <stdint.h>

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define MAX_LIMITED_LAT 79.9
#define MIN_LIMITED_LAT -79.9
#define A_Earth 6378137
#define e_Earth 0.08181919104281579

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define toRad(val) val * PI/180.0f
#define toDeg(val) val * 180.0f/PI

float distance_between(float lat1, float long1, float lat2, float long2);
float course_to(float lat1, float long1, float lat2, float long2);
uint16_t tilt_to(uint16_t distance, uint32_t alt1, uint32_t alt2);
void distance_move_to(float beginLat, float beginLon, float orient, float distance, float *distLat, float *distLon);