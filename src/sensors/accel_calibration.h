#ifndef __ACCEL_CALIBRATION_DEF_H__
#define __ACCEL_CALIBRATION_DEF_H__

#include <stdint.h>

extern void accel_calibration_init(void);
extern void accel_calibration_update(int16_t ax, int16_t ay, int16_t az);
extern void accel_calibration_finish(int16_t offsets[3], int16_t gains[3]);

#endif /* !__ACCEL_CALIBRATION_DEF_H__ */