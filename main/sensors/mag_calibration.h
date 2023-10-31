#ifndef __MAG_CALIBRATION_DEF_H__
#define __MAG_CALIBRATION_DEF_H__

#include <stdint.h>

extern void mag_calibration_init(void);
extern void mag_calibration_update(int16_t mx, int16_t my, int16_t mz);
extern void mag_calibration_finish(int16_t offsets[3]);

#endif /* !__MAG_CALIBRATION_DEF_H__ */