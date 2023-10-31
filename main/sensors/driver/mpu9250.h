#ifndef __MPU_9250_DEF_H__
#define __MPU_9250_DEF_H__

#include "../imu.h"
#include "mpu9250_i2c.h"

////////////////////////////////////////////////////////////////////////////////
//
// XXX refer to page 38 of MPU9250 Product specification
// for accel/gyro/mag orientation
//
//
//      chip orientation mark: top left
//
//      accel/gyro
//      X : left positive, counter clock
//      y : forward positive, counter clock
//      Z : up positive, counter clock
//
//      mag
//      x : forward positive
//      y : left positive
//      z : down positive
//
////////////////////////////////////////////////////////////////////////////////

#define MPU9250_I2C_ADDR        0x68      // or 110 100x. 400Khz interface

/* MPU9250 registers */
#define MPU9250_AUX_VDDIO             0x01
#define MPU9250_SMPLRT_DIV            0x19
#define MPU9250_REG_ACCEL_XOFFS_H     (0x06)
#define MPU9250_REG_ACCEL_XOFFS_L     (0x07)
#define MPU9250_REG_ACCEL_YOFFS_H     (0x08)
#define MPU9250_REG_ACCEL_YOFFS_L     (0x09)
#define MPU9250_REG_ACCEL_ZOFFS_H     (0x0A)
#define MPU9250_REG_ACCEL_ZOFFS_L     (0x0B)
#define MPU9250_REG_GYRO_XOFFS_H      (0x13)
#define MPU9250_REG_GYRO_XOFFS_L      (0x14)
#define MPU9250_REG_GYRO_YOFFS_H      (0x15)
#define MPU9250_REG_GYRO_YOFFS_L      (0x16)
#define MPU9250_REG_GYRO_ZOFFS_H      (0x17)
#define MPU9250_REG_GYRO_ZOFFS_L      (0x18)
#define MPU9250_CONFIG                0x1A
#define MPU9250_GYRO_CONFIG           0x1B
#define MPU9250_ACCEL_CONFIG          0x1C
#define MPU9250_MOTION_THRESH         0x1F
#define MPU9250_INT_PIN_CFG           0x37
#define MPU9250_INT_ENABLE            0x38
#define MPU9250_INT_STATUS            0x3A
#define MPU9250_ACCEL_XOUT_H          0x3B
#define MPU9250_ACCEL_XOUT_L          0x3C
#define MPU9250_ACCEL_YOUT_H          0x3D
#define MPU9250_ACCEL_YOUT_L          0x3E
#define MPU9250_ACCEL_ZOUT_H          0x3F
#define MPU9250_ACCEL_ZOUT_L          0x40
#define MPU9250_TEMP_OUT_H            0x41
#define MPU9250_TEMP_OUT_L            0x42
#define MPU9250_GYRO_XOUT_H           0x43
#define MPU9250_GYRO_XOUT_L           0x44
#define MPU9250_GYRO_YOUT_H           0x45
#define MPU9250_GYRO_YOUT_L           0x46
#define MPU9250_GYRO_ZOUT_H           0x47
#define MPU9250_GYRO_ZOUT_L           0x48
#define MPU9250_MOT_DETECT_STATUS     0x61
#define MPU9250_SIGNAL_PATH_RESET     0x68
#define MPU9250_MOT_DETECT_CTRL       0x69
#define MPU9250_USER_CTRL             0x6A
#define MPU9250_PWR_MGMT_1            0x6B
#define MPU9250_PWR_MGMT_2            0x6C
#define MPU9250_FIFO_COUNTH           0x72
#define MPU9250_FIFO_COUNTL           0x73
#define MPU9250_FIFO_R_W              0x74
#define MPU9250_WHO_AM_I              0x75
    
/* Gyro sensitivities in °/s */
#define MPU9250_GYRO_SENS_250       ((float) 131)
#define MPU9250_GYRO_SENS_500       ((float) 65.5)
#define MPU9250_GYRO_SENS_1000      ((float) 32.8)
#define MPU9250_GYRO_SENS_2000      ((float) 16.4)
    
/* Acce sensitivities in g */
#define MPU9250_ACCE_SENS_2         ((float) 16384)
#define MPU9250_ACCE_SENS_4         ((float) 8192)
#define MPU9250_ACCE_SENS_8         ((float) 4096)
#define MPU9250_ACCE_SENS_16        ((float) 2048)

#define AK8963_WIA                  0x00
#define AK8963_INFO                 0x01
#define AK8963_ST1                  0x02
#define AK8963_HXL                  0x03
#define AK8963_HXH                  0x04
#define AK8963_HYL                  0x05
#define AK8963_HYH                  0x06
#define AK8963_HZL                  0x07
#define AK8963_HZH                  0x08
#define AK8963_ST2                  0x09
#define AK8963_CNTL1                0x0a
#define AK8963_ASTC                 0x0c

#define AK8963_I2C_ADDR             0x0c
#define AK8963_MAG_LSB              0.15f     // in 16 bit mode, 0.15uT per LSB

typedef enum
{   
  MPU9250_Accelerometer_2G  = 0x00, /*!< Range is +- 2G */
  MPU9250_Accelerometer_4G  = 0x01, /*!< Range is +- 4G */
  MPU9250_Accelerometer_8G  = 0x02, /*!< Range is +- 8G */
  MPU9250_Accelerometer_16G = 0x03  /*!< Range is +- 16G */
} MPU9250_Accelerometer_t;

typedef enum 
{ 
  MPU9250_Gyroscope_250s  = 0x00,   /*!< Range is +- 250 degrees/s */
  MPU9250_Gyroscope_500s  = 0x01,   /*!< Range is +- 500 degrees/s */
  MPU9250_Gyroscope_1000s = 0x02,   /*!< Range is +- 1000 degrees/s */
  MPU9250_Gyroscope_2000s = 0x03    /*!< Range is +- 2000 degrees/s */
} MPU9250_Gyroscope_t; 

typedef struct
{
    mpu9250_i2c_config_t mpu9250_i2c_cfg;
    MPU9250_Accelerometer_t   accel_config;
    MPU9250_Gyroscope_t       gyro_config;
    bool available;
} mpu9250_t;

extern void mpu9250_init(mpu9250_t* mpu9250,
    MPU9250_Accelerometer_t accel_sensitivity,
    MPU9250_Gyroscope_t gyro_sensitivity,
    imu_raw_to_real_t* lsb);
extern bool mpu9250_is_available(mpu9250_t* mpu9250);
extern bool mpu9250_read_all(mpu9250_t* mpu9250, imu_sensor_data_t* data);
extern bool mpu9250_read_mag(mpu9250_t* mpu9250, imu_sensor_data_t* imu);
extern bool mpu9250_read_gyro_accel(mpu9250_t* mpu9250, imu_sensor_data_t* imu);

#endif /* !__MPU_9250_DEF_H__ */