/*
===========================================
IMU MODULE - MPU6050
-------------------------------------------
I2C CONNECTIONS:
SDA  -> GPIO21
SCL  -> GPIO22

I2C Address: 0x68
===========================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include "imu.h"

Adafruit_MPU6050 mpu;
static bool imuInitialized = false;

// Calibration offsets
static int16_t accelOffsetX = 0, accelOffsetY = 0, accelOffsetZ = 0;
static int16_t gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;

// Anomaly thresholds
#define ACCEL_ANOMALY_THRESHOLD 3.0   // 3g shock detection
#define GYRO_ANOMALY_THRESHOLD 300.0  // 300°/s rotation detection

void imu_init(void)
{
    Serial.println("[IMU] Initializing MPU6050...");

    if (!mpu.begin(0x68, &Wire))
    {
        Serial.println("[IMU] ERROR: MPU6050 not detected!");
        imuInitialized = false;
        return;
    }

    // Set accelerometer and gyroscope ranges
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);

    // Set filter bandwidth
    mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);

    imuInitialized = true;
    Serial.println("[IMU] MPU6050 initialized successfully");
}

bool imu_is_connected(void)
{
    return imuInitialized && mpu.begin(0x68, &Wire);
}

bool imu_read(imu_data_t *data)
{
    if (!imuInitialized)
        return false;

    sensors_event_t a, g, temp;

    mpu.getEvent(&a, &g, &temp);

    if (data)
    {
        data->ax = a.acceleration.x;
        data->ay = a.acceleration.y;
        data->az = a.acceleration.z;
        data->gx = g.gyro.x;
        data->gy = g.gyro.y;
        data->gz = g.gyro.z;
        data->temp = temp.temperature;

        return true;
    }

    return false;
}

imu_status_t imu_detect_anomaly(imu_data_t *data)
{
    imu_status_t status = {0.0, 0.0, false};

    if (!data)
        return status;

    // Calculate magnitude of acceleration
    status.accel_magnitude = sqrt(data->ax * data->ax +
                                 data->ay * data->ay +
                                 data->az * data->az);

    // Calculate magnitude of angular velocity
    status.gyro_magnitude = sqrt(data->gx * data->gx +
                                data->gy * data->gy +
                                data->gz * data->gz);

    // Detect anomalies based on thresholds
    if (status.accel_magnitude > ACCEL_ANOMALY_THRESHOLD ||
        status.gyro_magnitude > GYRO_ANOMALY_THRESHOLD)
    {
        status.anomaly_detected = true;
    }

    return status;
}

float imu_calculate_shock(imu_data_t *data)
{
    if (!data)
        return 0.0;

    // Calculate the magnitude of the acceleration vector
    float shock = sqrt(data->ax * data->ax +
                       data->ay * data->ay +
                       data->az * data->az);

    // Subtract 1g (gravity) to get actual shock
    float gravityMag = 1.0;
    float actualShock = shock - gravityMag;

    return actualShock > 0.0 ? actualShock : 0.0;
}