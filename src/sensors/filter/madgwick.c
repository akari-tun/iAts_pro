#include <math.h>
#include "math_helper.h"
#include "madgwick.h"

#define Q0 madgwick->q0
#define Q1 madgwick->q1
#define Q2 madgwick->q2
#define Q3 madgwick->q3

void madgwick_init(madgwick_t *madgwick, float sample_freq)
{
    madgwick->sampleFreq = sample_freq;

    madgwick->invSampleFreq = 1.0f / sample_freq;

    madgwick->beta = 0.25f;

    madgwick->q0 = 1.0f;
    madgwick->q1 = 0.0f;
    madgwick->q2 = 0.0f;
    madgwick->q3 = 0.0f;
}

void madgwick_update(madgwick_t *madgwick,
                     float gx, float gy, float gz,
                     float ax, float ay, float az,
                     float mx, float my, float mz)
{
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float hx, hy;
    float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

    // Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation)
    if ((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f))
    {
        madgwick_updateIMU(madgwick, gx, gy, gz, ax, ay, az);
        return;
    }

    // Convert gyroscope degrees/sec to radians/sec
    gx *= 0.0174533f;
    gy *= 0.0174533f;
    gz *= 0.0174533f;

    // Rate of change of quaternion from gyroscope
    qDot1 = 0.5f * (-Q1 * gx - Q2 * gy - Q3 * gz);
    qDot2 = 0.5f * (Q0 * gx + Q2 * gz - Q3 * gy);
    qDot3 = 0.5f * (Q0 * gy - Q1 * gz + Q3 * gx);
    qDot4 = 0.5f * (Q0 * gz + Q1 * gy - Q2 * gx);

    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))
    {
        // Normalise accelerometer measurement
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        // Normalise magnetometer measurement
        recipNorm = invSqrt(mx * mx + my * my + mz * mz);
        mx *= recipNorm;
        my *= recipNorm;
        mz *= recipNorm;

        // Auxiliary variables to avoid repeated arithmetic
        _2q0mx = 2.0f * Q0 * mx;
        _2q0my = 2.0f * Q0 * my;
        _2q0mz = 2.0f * Q0 * mz;
        _2q1mx = 2.0f * Q1 * mx;
        _2q0 = 2.0f * Q0;
        _2q1 = 2.0f * Q1;
        _2q2 = 2.0f * Q2;
        _2q3 = 2.0f * Q3;
        _2q0q2 = 2.0f * Q0 * Q2;
        _2q2q3 = 2.0f * Q2 * Q3;
        q0q0 = Q0 * Q0;
        q0q1 = Q0 * Q1;
        q0q2 = Q0 * Q2;
        q0q3 = Q0 * Q3;
        q1q1 = Q1 * Q1;
        q1q2 = Q1 * Q2;
        q1q3 = Q1 * Q3;
        q2q2 = Q2 * Q2;
        q2q3 = Q2 * Q3;
        q3q3 = Q3 * Q3;

        // Reference direction of Earth's magnetic field
        hx = mx * q0q0 - _2q0my * Q3 + _2q0mz * Q2 + mx * q1q1 + _2q1 * my * Q2 + _2q1 * mz * Q3 - mx * q2q2 - mx * q3q3;
        hy = _2q0mx * Q3 + my * q0q0 - _2q0mz * Q1 + _2q1mx * Q2 - my * q1q1 + my * q2q2 + _2q2 * mz * Q3 - my * q3q3;
        _2bx = sqrt(hx * hx + hy * hy);
        _2bz = -_2q0mx * Q2 + _2q0my * Q1 + mz * q0q0 + _2q1mx * Q3 - mz * q1q1 + _2q2 * my * Q3 - mz * q2q2 + mz * q3q3;
        _4bx = 2.0f * _2bx;
        _4bz = 2.0f * _2bz;

        // Gradient decent algorithm corrective step
        s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) + _2q1 * (2.0f * q0q1 + _2q2q3 - ay) - _2bz * Q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * Q3 + _2bz * Q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * Q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) + _2q0 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * Q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + _2bz * Q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * Q2 + _2bz * Q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * Q3 - _4bz * Q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) + _2q3 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * Q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + (-_4bx * Q2 - _2bz * Q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * Q1 + _2bz * Q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * Q0 - _4bz * Q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) + _2q2 * (2.0f * q0q1 + _2q2q3 - ay) + (-_4bx * Q3 + _2bz * Q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * Q0 + _2bz * Q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * Q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
        s0 *= recipNorm;
        s1 *= recipNorm;
        s2 *= recipNorm;
        s3 *= recipNorm;

        // Apply feedback step
        qDot1 -= madgwick->beta * s0;
        qDot2 -= madgwick->beta * s1;
        qDot3 -= madgwick->beta * s2;
        qDot4 -= madgwick->beta * s3;
    }

    // Integrate rate of change of quaternion to yield quaternion
    Q0 += qDot1 * madgwick->invSampleFreq;
    Q1 += qDot2 * madgwick->invSampleFreq;
    Q2 += qDot3 * madgwick->invSampleFreq;
    Q3 += qDot4 * madgwick->invSampleFreq;

    // Normalise quaternion
    recipNorm = invSqrt(Q0 * Q0 + Q1 * Q1 + Q2 * Q2 + Q3 * Q3);
    Q0 *= recipNorm;
    Q1 *= recipNorm;
    Q2 *= recipNorm;
    Q3 *= recipNorm;
}

void madgwick_updateIMU(madgwick_t *madgwick,
                        float gx, float gy, float gz,
                        float ax, float ay, float az)
{
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

    // Convert gyroscope degrees/sec to radians/sec
    gx *= 0.0174533f;
    gy *= 0.0174533f;
    gz *= 0.0174533f;

    // Rate of change of quaternion from gyroscope
    qDot1 = 0.5f * (-Q1 * gx - Q2 * gy - Q3 * gz);
    qDot2 = 0.5f * (Q0 * gx + Q2 * gz - Q3 * gy);
    qDot3 = 0.5f * (Q0 * gy - Q1 * gz + Q3 * gx);
    qDot4 = 0.5f * (Q0 * gz + Q1 * gy - Q2 * gx);

    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))
    {
        // Normalise accelerometer measurement
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        // Auxiliary variables to avoid repeated arithmetic
        _2q0 = 2.0f * Q0;
        _2q1 = 2.0f * Q1;
        _2q2 = 2.0f * Q2;
        _2q3 = 2.0f * Q3;
        _4q0 = 4.0f * Q0;
        _4q1 = 4.0f * Q1;
        _4q2 = 4.0f * Q2;
        _8q1 = 8.0f * Q1;
        _8q2 = 8.0f * Q2;
        q0q0 = Q0 * Q0;
        q1q1 = Q1 * Q1;
        q2q2 = Q2 * Q2;
        q3q3 = Q3 * Q3;

        // Gradient decent algorithm corrective step
        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
        s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * Q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
        s2 = 4.0f * q0q0 * Q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
        s3 = 4.0f * q1q1 * Q3 - _2q1 * ax + 4.0f * q2q2 * Q3 - _2q2 * ay;
        recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
        s0 *= recipNorm;
        s1 *= recipNorm;
        s2 *= recipNorm;
        s3 *= recipNorm;

        // Apply feedback step
        qDot1 -= madgwick->beta * s0;
        qDot2 -= madgwick->beta * s1;
        qDot3 -= madgwick->beta * s2;
        qDot4 -= madgwick->beta * s3;
    }

    // Integrate rate of change of quaternion to yield quaternion
    Q0 += qDot1 * madgwick->invSampleFreq;
    Q1 += qDot2 * madgwick->invSampleFreq;
    Q2 += qDot3 * madgwick->invSampleFreq;
    Q3 += qDot4 * madgwick->invSampleFreq;

    // Normalise quaternion
    recipNorm = invSqrt(Q0 * Q0 + Q1 * Q1 + Q2 * Q2 + Q3 * Q3);
    Q0 *= recipNorm;
    Q1 *= recipNorm;
    Q2 *= recipNorm;
    Q3 *= recipNorm;
}

void madgwick_get_roll_pitch_yaw(madgwick_t *madgwick, float data[3], float md)
{
    float roll, pitch, yaw;

    roll = atan2f(Q0 * Q1 + Q2 * Q3, 0.5f - Q1 * Q1 - Q2 * Q2);
    pitch = asinf(-2.0f * (Q1 * Q3 - Q0 * Q2));
    yaw = atan2f(Q1 * Q2 + Q0 * Q3, 0.5f - Q2 * Q2 - Q3 * Q3);

    data[0] = roll * 57.29578f;
    data[1] = pitch * 57.29578f;
    data[2] = yaw * 57.29578f + md;

    if (data[2] < 0.0f)
    {
        data[2] += 360.0f;
    }
}

void madgwick_get_quaternion(madgwick_t *madgwick, float data[4])
{
    data[0] = Q0;
    data[1] = Q1;
    data[2] = Q2;
    data[3] = Q3;
}