#ifndef __IMU_TASK_DEF_H__
#define __IMU_TASK_DEF_H__

#include "imu.h"
#include "driver/mpu9250.h"

extern void imu_task_init(hal_i2c_config_t *i2c_cfg);

extern void imu_task_do_mag_calibration(void);
extern void imu_task_do_gyro_calibration(void);

extern void imu_task_do_accel_calibration_init(void);
extern void imu_task_do_accel_calibration_start(void);
extern void imu_task_do_accel_calibration_finish(void);

extern void imu_task_get_raw_and_data(imu_mode_t* mode,
    imu_sensor_data_t* raw,
    imu_sensor_data_t* calibrated,
    imu_data_t* data);

extern uint32_t imu_task_get_loop_cnt(void);

extern void imu_task_get_cal_state(imu_sensor_calib_data_t* cal);

extern void imu_task_get_mag_calibration(imu_mode_t* mode,
    int16_t raw[3],
    int16_t calibrated[3],
    int16_t mag_bias[3]);

extern imu_t *imu_task_get();

#endif /* !__IMU_TASK_DEF_H__ */