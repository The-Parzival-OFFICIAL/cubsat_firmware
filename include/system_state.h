#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <cstdint>
#include <ctime>

// ============================================
// SYSTEM STATE & OPERATIONAL MODES
// ============================================

typedef enum {
    MODE_NORMAL = 0,    // All systems active
    MODE_SAFE = 1,      // Reduced operations, fault recovery
    MODE_CRITICAL = 2   // Emergency shutdown of non-essential systems
} system_mode_t;

typedef enum {
    FAULT_NONE = 0x00,
    FAULT_TEMP_HIGH = 0x01,
    FAULT_TEMP_LOW = 0x02,
    FAULT_VOLTAGE_LOW = 0x04,
    FAULT_VOLTAGE_HIGH = 0x08,
    FAULT_CURRENT_HIGH = 0x10,
    FAULT_IMU_FAIL = 0x20,
    FAULT_LORA_FAIL = 0x40,
    FAULT_I2C_ERROR = 0x80
} fault_flag_t;

typedef struct {
    // Power measurements
    float voltage;
    float current;
    float power;

    // Temperature
    float temperature;

    // IMU data
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;

    // System state
    uint8_t fault_flags;
    system_mode_t mode;
    uint32_t uptime_seconds;
    
    // Telemetry counters
    uint32_t cycles_normal;
    uint32_t cycles_safe;
    uint32_t cycles_critical;
    uint32_t recovery_attempts;
} system_status_t;

// Global system status
extern system_status_t systemStatus;

// Mutex for thread-safe access
extern SemaphoreHandle_t stateMutex;

#endif