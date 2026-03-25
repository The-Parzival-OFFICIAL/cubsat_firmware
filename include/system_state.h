#include <cstdint>

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

typedef enum {
    MODE_NORMAL = 0,
    MODE_SAFE
} system_mode_t;

typedef struct {
    float voltage;
    float current;
    uint8_t fault_flags;
    system_mode_t mode;
} system_status_t;

extern system_status_t systemStatus;

#endif