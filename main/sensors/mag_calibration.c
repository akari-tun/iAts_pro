#include <math.h>

#include "mag_calibration.h"
#include "sensor_calib.h"

static int32_t _mag_prev[3];
static int16_t _mag_offset[3];

static sensor_calib_t _cal_state;

void mag_calibration_init(void)
{
    _mag_prev[0] =
        _mag_prev[1] =
            _mag_prev[2] = 0;

    sensorCalibrationResetState(&_cal_state);
}

void mag_calibration_update(int16_t mx, int16_t my, int16_t mz)
{
    float diffMag = 0;
    float avgMag = 0;
    int32_t mag_data[3];

    mag_data[0] = mx;
    mag_data[1] = my;
    mag_data[2] = mz;

    for (int axis = 0; axis < 3; axis++)
    {
        diffMag += (mag_data[axis] - _mag_prev[axis]) * (mag_data[axis] - _mag_prev[axis]);
        avgMag += (mag_data[axis] + _mag_prev[axis]) * (mag_data[axis] + _mag_prev[axis]) / 4.0f;
    }

    // sqrtf(diffMag / avgMag) is a rough approximation of tangent of angle between magADC and _mag_prev. tan(8 deg) = 0.14
    if ((avgMag > 0.01f) && ((diffMag / avgMag) > (0.14f * 0.14f)))
    {
        sensorCalibrationPushSampleForOffsetCalculation(&_cal_state, mag_data);

        for (int axis = 0; axis < 3; axis++)
        {
            _mag_prev[axis] = mag_data[axis];
        }
    }
}

void mag_calibration_finish(int16_t offsets[3])
{
    float magZerof[3];

    sensorCalibrationSolveForOffset(&_cal_state, magZerof);

    for (int axis = 0; axis < 3; axis++)
    {
        _mag_offset[axis] = lrintf(magZerof[axis]);
    }

    offsets[0] = _mag_offset[0];
    offsets[1] = _mag_offset[1];
    offsets[2] = _mag_offset[2];
}