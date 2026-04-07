#include "system_state.h"

// Global system status initialized to safe defaults
system_status_t systemStatus = {
    // Power
    .voltage = 0.0,
    .current = 0.0,
    .power = 0.0,

    // Temperature
    .temperature = 0.0,

    // IMU
    .accel_x = 0.0, .accel_y = 0.0, .accel_z = 0.0,
    .gyro_x = 0.0, .gyro_y = 0.0, .gyro_z = 0.0,

    // System state
    .fault_flags = 0x00,
    .mode = MODE_NORMAL,
    .uptime_seconds = 0,

    // Counters
    .cycles_normal = 0,
    .cycles_safe = 0,
    .cycles_critical = 0,
    .recovery_attempts = 0
};