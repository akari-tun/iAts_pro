#ifndef __SENSOR_CALIB_DEF_H__
#define __SENSOR_CALIB_DEF_H__

/**
 *
 * Sensor offset calculation code based on Freescale's AN4246
 *
 */

typedef struct
{
    float XtY[4];
    float XtX[4][4];
} sensor_calib_t;

void sensorCalibrationResetState(sensor_calib_t *state);
void sensorCalibrationPushSampleForOffsetCalculation(sensor_calib_t *state, int32_t sample[3]);
void sensorCalibrationPushSampleForScaleCalculation(sensor_calib_t *state, int axis, int32_t sample[3], int target);
void sensorCalibrationSolveForOffset(sensor_calib_t *state, float result[3]);
void sensorCalibrationSolveForScale(sensor_calib_t *state, float result[3]);

#endif /* !__SENSOR_CALIB_DEF_H__ */