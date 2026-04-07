#ifndef HEALTH_H
#define HEALTH_H

#include <cstdint>

// ============================================
// HEALTH MONITORING & FAULT DETECTION
// ============================================

typedef enum {
    FAULT_NONE = 0x00,
    FAULT_TEMP_HIGH = 0x01,
    FAULT_TEMP_LOW = 0x02,
    FAULT_VOLTAGE_LOW = 0x04,
    FAULT_VOLTAGE_HIGH = 0x08,
    FAULT_CURRENT_HIGH = 0x10,
    FAULT_MPU6050_FAIL = 0x20,
    FAULT_LORA_FAIL = 0x40,
    FAULT_I2C_ERROR = 0x80
} fault_flag_t;

typedef enum {
    RECOVERY_IDLE = 0,
    RECOVERY_IN_PROGRESS = 1,
    RECOVERY_SUCCESS = 2,
    RECOVERY_FAILED = 3
} recovery_state_t;

typedef struct {
    float temp_max;
    float temp_min;
    float voltage_max;
    float voltage_min;
    float current_max;
} safety_thresholds_t;

// Function declarations
void health_init(void);
void health_task(void *param);
void health_check_and_recover(void);
uint8_t health_get_faults(void);
void health_clear_fault(fault_flag_t fault);
recovery_state_t health_get_recovery_state(void);
void health_set_recovery_state(recovery_state_t state);
const char* health_fault_to_string(fault_flag_t fault);

#endif