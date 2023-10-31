#include <math.h>
#include <stdlib.h>
#include "accel_calibration.h"
#include "sensor_calib.h"
#include <hal/log.h>
/*
   As for accelerometer, things are not that simple. The purpose of accelerometer calibration
   is to find offset and scale for the sensor so that actual accelerometer value is
   calculated using the following fomula.
      actual = (raw - offset) * scale
   To calculate offset/scale, we use a typical 6 axis sampling/calibration mechanism.
*/

const static char *TAG = "ACC_CAL";

static int32_t _sample_count[6];
static int32_t _acc_sum[6][3];
static int32_t _offset[3];
static int32_t _gain[3];

static sensor_calib_t _cal_state;

#define ACCEL_GAIN_REF (512 * 8)

static int8_t _axis_idx = -1;

static int getPrimaryAxisIndex(int32_t sample[3])
{
    const int x = 0,
              y = 1,
              z = 2;

    // tolerate up to atan(1 / 1.5) = 33 deg tilt (in worst case 66 deg separation between points)
    if ((abs(sample[z]) / 1.5f) > abs(sample[x]) && (abs(sample[z]) / 1.5f) > abs(sample[y]))
    {
      //z-axis
        return (sample[z] > 0) ? 0 : 1;
    }
    else if ((abs(sample[x]) / 1.5f) > abs(sample[y]) && (abs(sample[x]) / 1.5f) > abs(sample[z]))
    {
      //x-axis
        return (sample[x] > 0) ? 2 : 3;
    }
    else if ((abs(sample[y]) / 1.5f) > abs(sample[x]) && (abs(sample[y]) / 1.5f) > abs(sample[z]))
    {
      //y-axis
        return (sample[y] > 0) ? 4 : 5;
    }
    else
    {
        return -1;
    }
}

void accel_calibration_init(void)
{
    for (int i = 0; i < 6; i++)
    {
        _acc_sum[i][0] = 0;
        _acc_sum[i][1] = 0;
        _acc_sum[i][2] = 0;

        _sample_count[i] = 0;
    }

    _axis_idx = -1;

    sensorCalibrationResetState(&_cal_state);
}

void accel_calibration_update(int16_t ax, int16_t ay, int16_t az)
{
    int32_t accel_value[3];
    int axis_ndx;

    accel_value[0] = ax;
    accel_value[1] = ay;
    accel_value[2] = az;

    axis_ndx = getPrimaryAxisIndex(accel_value);

    if (axis_ndx < 0)
    {
        return;
    }

    if (_axis_idx != axis_ndx)
    {
        _axis_idx = axis_ndx;
        LOG_I(TAG, "Calibration Axis [%d] x[%d] y[%d] z[%d]", _axis_idx, ax, ay, az);
    }
    
    sensorCalibrationPushSampleForOffsetCalculation(&_cal_state, accel_value);

    _acc_sum[axis_ndx][0] += accel_value[0];
    _acc_sum[axis_ndx][1] += accel_value[1];
    _acc_sum[axis_ndx][2] += accel_value[2];

    _sample_count[axis_ndx]++;
}

void accel_calibration_finish(int16_t offsets[3], int16_t gains[3])
{
    float tmp[3];
    int32_t sample[3];

    /* calculate offset */
    sensorCalibrationSolveForOffset(&_cal_state, tmp);

    for (int i = 0; i < 3; i++)
    {
        _offset[i] = lrintf(tmp[i]);
    }

    /* Not we can offset our accumulated averages samples and calculate scale factors and calculate gains */
    sensorCalibrationResetState(&_cal_state);

    for (int axis = 0; axis < 6; axis++)
    {
        LOG_I(TAG, "axis[%d] ->  _acc_sum[0] = %d | _acc_sum[1] = %d | _acc_sum[2] = %d | _sample_count = %d", axis, _acc_sum[axis][0], _acc_sum[axis][1], _acc_sum[axis][2], _sample_count[axis]);
    }

    for (int axis = 0; axis < 6; axis++)
    {
        if (_sample_count[axis] != 0)
        {
            sample[0] = _acc_sum[axis][0] / _sample_count[axis] - _offset[0];
            sample[1] = _acc_sum[axis][1] / _sample_count[axis] - _offset[1];
            sample[2] = _acc_sum[axis][2] / _sample_count[axis] - _offset[2];

            sensorCalibrationPushSampleForScaleCalculation(&_cal_state, axis / 2, sample, ACCEL_GAIN_REF);
        }
    }

    sensorCalibrationSolveForScale(&_cal_state, tmp);

    for (int axis = 0; axis < 3; axis++)
    {
        _gain[axis] = lrintf(tmp[axis] * 4096);
    }

    offsets[0] = _offset[0];
    offsets[1] = _offset[1];
    offsets[2] = _offset[2];

    gains[0] = _gain[0];
    gains[1] = _gain[1];
    gains[2] = _gain[2];
}