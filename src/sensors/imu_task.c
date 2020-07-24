#include <stdio.h>
#include <sys/time.h>
#include <string.h>
// #include "generic_list.h"
// #include "shell.h"
// #include "io_driver.h"
#include <hal/log.h>
//#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "imu_task.h"
#include "driver/mpu9250.h"

#include "config/settings.h"

#include "platform/storage.h"
#include "protocols/atp.h"

// #include "sdkconfig.h"

#define IMU_POLL_INTERVAL 2

const static char *TAG = "imu_task";

#define IMU_STORAGE_KEY "calibrations"

typedef enum
{
    imu_task_perform_mag_calibration,
    imu_task_perform_gyro_calibration,
    imu_task_perform_accel_calibration_init,
    imu_task_perform_accel_calibration_step,
    imu_task_perform_accel_calibration_finish,
} imu_task_command_t;

static SemaphoreHandle_t _mutex;
static imu_t _imu;
static mpu9250_t _mpu9250;
static volatile uint32_t _loop_cnt = 0;
static xQueueHandle _cmd_queue = NULL;
static storage_t storage;

static void imu_task_load_calibration(void)
{
    //esp_err_t err;
    //nvs_handle nvs_handle;
    static const imu_sensor_calib_data_t cal_default =
        {
            .accel_off = {0, 0, 0},
            .accel_scale = {4096, 4096, 4096},
            .gyro_off = {0, 0, 0},
            .mag_bias = {0, 0, 0},
            .mag_declination = 0.0f
        };

    //err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    // if (err != ESP_OK)
    // {
    //     memcpy(&_imu.cal, &cal_default, sizeof(imu_sensor_calib_data_t));
    //     ESP_LOGI(TAG, "nvs_open failed. going with all defaults");
    //     return;
    // }

    if (storage_get_i16(&storage, "accel_off_x", &_imu.cal.accel_off[0]))
    {
        _imu.cal.accel_off[0] = cal_default.accel_off[0];
    }

    if (storage_get_i16(&storage, "accel_off_y", &_imu.cal.accel_off[1]))
    {
        _imu.cal.accel_off[1] = cal_default.accel_off[1];
    }

    if (storage_get_i16(&storage, "accel_off_z", &_imu.cal.accel_off[2]))
    {
        _imu.cal.accel_off[2] = cal_default.accel_off[2];
    }

    if (storage_get_i16(&storage, "accel_scale_x", &_imu.cal.accel_scale[0]))
    {
        _imu.cal.accel_scale[0] = cal_default.accel_scale[0];
    }

    if (storage_get_i16(&storage, "accel_scale_y", &_imu.cal.accel_scale[1]))
    {
        _imu.cal.accel_scale[1] = cal_default.accel_scale[1];
    }

    if (storage_get_i16(&storage, "accel_scale_z", &_imu.cal.accel_scale[2]))
    {
        _imu.cal.accel_scale[2] = cal_default.accel_scale[2];
    }

    if (storage_get_i16(&storage, "gyro_off_x", &_imu.cal.gyro_off[0]))
    {
        _imu.cal.gyro_off[0] = cal_default.gyro_off[0];
    }

    if (storage_get_i16(&storage, "gyro_off_y", &_imu.cal.gyro_off[1]))
    {
        _imu.cal.gyro_off[1] = cal_default.gyro_off[1];
    }

    if (storage_get_i16(&storage, "gyro_off_z", &_imu.cal.gyro_off[2]))
    {
        _imu.cal.gyro_off[2] = cal_default.gyro_off[2];
    }

    if (storage_get_i16(&storage, "mag_bias_x", &_imu.cal.mag_bias[0]))
    {
        _imu.cal.mag_bias[0] = cal_default.mag_bias[0];
    }

    if (storage_get_i16(&storage, "mag_bias_y", &_imu.cal.mag_bias[1]))
    {
        _imu.cal.mag_bias[1] = cal_default.mag_bias[1];
    }

    if (storage_get_i16(&storage, "mag_bias_z", &_imu.cal.mag_bias[2]))
    {
        _imu.cal.mag_bias[2] = cal_default.mag_bias[2];
    }
}

static void imu_task_save_calibration(void)
{
    // esp_err_t err;
    // nvs_handle nvs_handle;
    // err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "nvs_open failed. saving calibration data failed");
    // }

    storage_set_i16(&storage, "accel_off_x", _imu.cal.accel_off[0]);
    storage_set_i16(&storage, "accel_off_y", _imu.cal.accel_off[1]);
    storage_set_i16(&storage, "accel_off_z", _imu.cal.accel_off[2]);
    storage_set_i16(&storage, "accel_scale_x", _imu.cal.accel_scale[0]);
    storage_set_i16(&storage, "accel_scale_y", _imu.cal.accel_scale[1]);
    storage_set_i16(&storage, "accel_scale_z", _imu.cal.accel_scale[2]);
    storage_set_i16(&storage, "gyro_off_x", _imu.cal.gyro_off[0]);
    storage_set_i16(&storage, "gyro_off_y", _imu.cal.gyro_off[1]);
    storage_set_i16(&storage, "gyro_off_z", _imu.cal.gyro_off[2]);
    storage_set_i16(&storage, "mag_bias_x", _imu.cal.mag_bias[0]);
    storage_set_i16(&storage, "mag_bias_y", _imu.cal.mag_bias[1]);
    storage_set_i16(&storage, "mag_bias_z", _imu.cal.mag_bias[2]);

    storage_commit(&storage);
}

static void imu_task(void *pvParameters)
{
    const TickType_t xDelay = IMU_POLL_INTERVAL / portTICK_PERIOD_MS;
    uint32_t cmd;
    struct timeval cal_start_time,
        now;
    int cnt = 0;

    LOG_I(TAG, "starting imu task");

    mpu9250_init(&_mpu9250, MPU9250_Accelerometer_8G, MPU9250_Gyroscope_1000s, &_imu.lsb);
    _imu.available = mpu9250_is_available(&_mpu9250);

    if (!_imu.available)
    {
        imu_disable();
        LOG_I(TAG, "imu_disable");
    }

    while (_imu.available)
    {
        if (xQueueReceive(_cmd_queue, &cmd, xDelay))
        {
            switch (cmd)
            {
            case imu_task_perform_mag_calibration:
                gettimeofday(&cal_start_time, NULL);
                imu_mag_calibration_start(&_imu);
                break;

            case imu_task_perform_gyro_calibration:
                gettimeofday(&cal_start_time, NULL);
                imu_gyro_calibration_start(&_imu);
                break;

            case imu_task_perform_accel_calibration_init:
                imu_accel_calibration_init(&_imu);
                break;

            case imu_task_perform_accel_calibration_step:
                gettimeofday(&cal_start_time, NULL);
                imu_accel_calibration_step_start(&_imu);
                break;

            case imu_task_perform_accel_calibration_finish:
                imu_accel_calibration_finish(&_imu);
                imu_task_save_calibration();
                break;

            default:
                break;
            }
        }

        xSemaphoreTake(_mutex, portMAX_DELAY);

        // mpu9250_read_all(&_mpu9250, &_imu.raw);

        xSemaphoreTake(_mpu9250.mpu9250_i2c_cfg.i2c_cfg->xSemaphore, portMAX_DELAY);
        {
            mpu9250_read_gyro_accel(&_mpu9250, &_imu.raw);

            if (cnt == 0) mpu9250_read_mag(&_mpu9250, &_imu.raw);
            cnt++;
            if (cnt >= 25) cnt = 0;
        }
        xSemaphoreGive(_mpu9250.mpu9250_i2c_cfg.i2c_cfg->xSemaphore);

        imu_update(&_imu);

        ATP_SET_FLOAT(TAG_TRACKER_ROLL, _imu.data.orientation[0], time_micros_now());
        ATP_SET_FLOAT(TAG_TRACKER_PITCH, _imu.data.orientation[1], time_micros_now());
        ATP_SET_FLOAT(TAG_TRACKER_YAW, _imu.data.orientation[2], time_micros_now());

        if (_imu.mode != imu_mode_normal)
        {
            gettimeofday(&now, NULL);

            switch (_imu.mode)
            {
            case imu_mode_mag_calibrating:
                _imu.calibration_time = 60 - (now.tv_sec - cal_start_time.tv_sec);
                //if ((now.tv_sec - cal_start_time.tv_sec) >= 60)
                if (_imu.calibration_time <= 0)
                {
                    imu_mag_calibration_finish(&_imu);
                    imu_task_save_calibration();
                }
                break;

            case imu_mode_gyro_calibrating:
                _imu.calibration_time = 30 - (now.tv_sec - cal_start_time.tv_sec);
                //if ((now.tv_sec - cal_start_time.tv_sec) >= 30)
                if (_imu.calibration_time <= 0)
                {
                    imu_gyro_calibration_finish(&_imu);
                    imu_task_save_calibration();
                }
                break;

            case imu_mode_accel_calibrating:
                _imu.calibration_time = 10 - (now.tv_sec - cal_start_time.tv_sec);
                //if ((now.tv_sec - cal_start_time.tv_sec) >= 20)
                if (_imu.calibration_time <= 0)
                {
                    imu_accel_calibration_step_stop(&_imu);
                }
                break;

            default:
                _imu.calibration_time = 0;
                break;
            }
        }

        xSemaphoreGive(_mutex);
        _loop_cnt++;
    }
}

void imu_task_init(hal_i2c_config_t *i2c_cfg)
{
    LOG_I(TAG, "initialing IMU task");

    if (settings_get_key_bool(SETTING_KEY_IMU_ENABLE))
    {
        storage_init(&storage, IMU_STORAGE_KEY);

        _mpu9250.mpu9250_i2c_cfg.i2c_cfg = i2c_cfg;
        _mpu9250.mpu9250_i2c_cfg.rst = HAL_GPIO_NONE;
        _mpu9250.mpu9250_i2c_cfg.addr = MPU9250_I2C_ADDR;
        _mpu9250.mpu9250_i2c_cfg.ak8963_addr = AK8963_I2C_ADDR;

        imu_init(&_imu);

        imu_task_load_calibration();

        _mutex = xSemaphoreCreateMutex();

        _cmd_queue = xQueueCreate(10, sizeof(uint32_t));

        xTaskCreatePinnedToCore(imu_task, "IMU_Task", 4096, NULL, 1, NULL, 0);
    }
}

void imu_task_get_raw_and_data(imu_mode_t *mode,
                               imu_sensor_data_t *raw,
                               imu_sensor_data_t *calibrated,
                               imu_data_t *data)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);

    *mode = _imu.mode;
    memcpy(raw, &_imu.raw, sizeof(imu_sensor_data_t));
    memcpy(calibrated, &_imu.adjusted, sizeof(imu_sensor_data_t));
    memcpy(data, &_imu.data, sizeof(imu_data_t));

    xSemaphoreGive(_mutex);
}

void imu_task_get_mag_calibration(imu_mode_t *mode,
                                  int16_t raw[3],
                                  int16_t calibrated[3],
                                  int16_t mag_bias[3])
{
    xSemaphoreTake(_mutex, portMAX_DELAY);

    *mode = _imu.mode;

    raw[0] = _imu.raw.mag[0];
    raw[1] = _imu.raw.mag[1];
    raw[2] = _imu.raw.mag[2];

    calibrated[0] = _imu.adjusted.mag[0];
    calibrated[1] = _imu.adjusted.mag[1];
    calibrated[2] = _imu.adjusted.mag[2];

    mag_bias[0] = _imu.cal.mag_bias[0];
    mag_bias[1] = _imu.cal.mag_bias[1];
    mag_bias[2] = _imu.cal.mag_bias[2];

    xSemaphoreGive(_mutex);
}

void imu_task_get_cal_state(imu_sensor_calib_data_t *cal)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);

    memcpy(cal, &_imu.cal, sizeof(imu_sensor_calib_data_t));

    xSemaphoreGive(_mutex);
}

uint32_t imu_task_get_loop_cnt(void)
{
    return _loop_cnt;
}

void imu_task_do_mag_calibration(void)
{
    imu_task_command_t cmd = imu_task_perform_mag_calibration;

    xQueueSend(_cmd_queue, &cmd, 1000 / portTICK_PERIOD_MS);
}

void imu_task_do_gyro_calibration(void)
{
    imu_task_command_t cmd = imu_task_perform_gyro_calibration;

    xQueueSend(_cmd_queue, &cmd, 1000 / portTICK_PERIOD_MS);
}

void imu_task_do_accel_calibration_init(void)
{
    imu_task_command_t cmd = imu_task_perform_accel_calibration_init;
    _imu.calibration_acc_step = 0;
    _imu.calibration_time = 0;
    xQueueSend(_cmd_queue, &cmd, 1000 / portTICK_PERIOD_MS);
}

void imu_task_do_accel_calibration_start(void)
{
    imu_task_command_t cmd = imu_task_perform_accel_calibration_step;

    xQueueSend(_cmd_queue, &cmd, 1000 / portTICK_PERIOD_MS);
}

void imu_task_do_accel_calibration_finish(void)
{
    imu_task_command_t cmd = imu_task_perform_accel_calibration_finish;

    xQueueSend(_cmd_queue, &cmd, 1000 / portTICK_PERIOD_MS);
}

imu_t *imu_task_get()
{
    return &_imu;
}