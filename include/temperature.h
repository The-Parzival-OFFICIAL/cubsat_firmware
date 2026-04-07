#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <cstdint>

// ============================================
// TEMPERATURE SENSOR MODULE
// Supports: LM35DZ (Analog), DS18B20 (1-Wire)
// ============================================

typedef enum {
    TEMP_SENSOR_NONE = -1,
    TEMP_SENSOR_LM35DZ = 0,
    TEMP_SENSOR_DS18B20 = 1
} temp_sensor_type_t;

// Function declarations
void temperature_init(temp_sensor_type_t sensor_type, uint8_t pin);
float temperature_read(void);
bool temperature_is_valid(void);
float temperature_get_last(void);

#endif