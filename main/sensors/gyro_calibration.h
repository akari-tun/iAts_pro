#ifndef __GYRO_CALIBRATION_DEF_H__
#define __GYRO_CALIBRATION_DEF_H__

#include <stdint.h>

extern void gyro_calibration_init(void);
extern void gyro_calibration_update(int16_t gx, int16_t gy, int16_t gz);
extern void gyro_calibration_finish(int16_t offsets[3]);

#endif /* !__GYRO_CALIBRATION_DEF_H__ */