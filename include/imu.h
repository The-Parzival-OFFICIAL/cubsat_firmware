#ifndef IMU_H
#define IMU_H

#include <cstdint>

// ============================================
// INERTIAL MEASUREMENT UNIT - MPU6050
// -------------------------------------------
// I2C CONNECTIONS:
// SDA -> GPIO21
// SCL -> GPIO22
// Address: 0x68
//
// Provides:
// - 3-axis accelerometer (±2g to ±16g)
// - 3-axis gyroscope (±250°/s to ±2000°/s)
// ============================================

typedef struct {
    float ax, ay, az;  // Acceleration (g)
    float gx, gy, gz;  // Angular velocity (°/s)
    float temp;        // Temperature (°C)
} imu_data_t;

typedef struct {
    float accel_magnitude;
    float gyro_magnitude;
    bool anomaly_detected;
} imu_status_t;

// Function declarations
void imu_init(void);
bool imu_is_connected(void);
bool imu_read(imu_data_t *data);
imu_status_t imu_detect_anomaly(imu_data_t *data);
float imu_calculate_shock(imu_data_t *data);

#endif