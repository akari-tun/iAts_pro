#ifndef __IMU_DEF_H__
#define __IMU_DEF_H__

#include <stdbool.h>
#include <stdint.h>
#include "filter/madgwick.h"
#include "filter/mahony.h"

#define USE_MADGWICK 1

#define IMU_POLL_INTERVAL 2

typedef enum
{
    imu_mode_normal,
    imu_mode_accel_calibrating,
    imu_mode_gyro_calibrating,
    imu_mode_mag_calibrating,
} imu_mode_t;

typedef enum
{
    imu_board_align_cw_0,
    imu_board_align_cw_90,
    imu_board_align_cw_180,
    imu_board_align_cw_270,
    imu_board_align_cw_0_flip,
    imu_board_align_cw_90_flip,
    imu_board_align_cw_180_flip,
    imu_board_align_cw_270_flip,
    imu_board_align_special,
    imu_board_align_special2
} imu_board_align_t;

typedef struct
{
    int16_t accel[3];
    int16_t gyro[3];
    int16_t mag[3];
    int16_t temp;
} imu_sensor_data_t;

typedef struct
{
    int16_t accel_off[3];
    int16_t accel_scale[3];
    int16_t gyro_off[3];
    int16_t mag_bias[3];
    float mag_declination;
} imu_sensor_calib_data_t;

typedef struct
{
    float accel_lsb;
    float gyro_lsb;
    float mag_lsb;
} imu_raw_to_real_t;

typedef struct
{
    float accel[3];       // in G
    float gyro[3];        // in degrees per sec  (not radian)
    float mag[3];         // in uT
    float temp;           // in celcius
    float orientation[3]; // AHRS output
    float quaternion[4];
} imu_data_t;

typedef struct
{
    imu_mode_t mode;

    imu_sensor_data_t raw;
    imu_sensor_data_t adjusted;

    imu_sensor_calib_data_t cal;

    imu_board_align_t accel_align;
    imu_board_align_t gyro_align;
    imu_board_align_t mag_align;

    imu_raw_to_real_t lsb;

    imu_data_t data;

#if USE_MADGWICK == 1
    madgwick_t filter;
#else
    mahony_t filter;
#endif
    int32_t calibration_time;
    int8_t calibration_acc_step;
    float update_rate;
    bool enable;
    bool available;
} imu_t;

extern void imu_init(imu_t *imu);
extern void imu_update(imu_t *imu);
extern bool imu_is_available(imu_t *imu);
extern void imu_disable();

extern void imu_mag_calibration_start(imu_t *imu);
extern void imu_mag_calibration_finish(imu_t *imu);

extern void imu_gyro_calibration_start(imu_t *imu);
extern void imu_gyro_calibration_finish(imu_t *imu);

extern void imu_accel_calibration_init(imu_t *imu);
extern void imu_accel_calibration_step_start(imu_t *imu);
extern void imu_accel_calibration_step_stop(imu_t *imu);
extern void imu_accel_calibration_finish(imu_t *imu);

#endif /* !__IMU_DEF_H__ */