#ifndef POWER_H
#define POWER_H

#include <cstdint>

// ===============================================
// POWER MODULE - INA219 POWER MONITOR
// ===============================================
// I2C CONNECTIONS:
// SDA  -> GPIO21
// SCL  -> GPIO22
// Address: 0x40 (default)
//
// Operating Voltage: 3.3V
// Calibration: 32V / 2A Mode
// Max Measurable Voltage: 32V
// Max Measurable Current: 2A (2000mA)
// ===============================================

// INA219 Configuration Constants
#define INA219_I2C_ADDR 0x40          // Default I2C address
#define INA219_CALIB_32V_2A 4096      // Calibration value for 32V/2A

// Register addresses
#define INA219_REG_CONFIG 0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE 0x02
#define INA219_REG_POWER 0x03
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

// Power measurement thresholds (Safety limits)
#define POWER_VOLTAGE_MAX 4.5     // 4.5V overvoltage limit
#define POWER_VOLTAGE_MIN 2.5     // 2.5V brownout limit
#define POWER_CURRENT_MAX 2000.0  // 2000mA overcurrent limit
#define POWER_SAMPLE_COUNT 5      // Number of samples to average

// Typedef for power readings
typedef struct {
    float bus_voltage;      // Bus voltage in volts
    float shunt_voltage;    // Shunt voltage in millivolts
    float current_mA;       // Current in milliamps
    float power_mW;         // Power in milliwatts
    uint32_t timestamp_ms;  // Timestamp of measurement
    bool is_valid;          // Measurement validity flag
} power_reading_t;

// Typedef for power statistics
typedef struct {
    float voltage_avg;      // Average voltage
    float voltage_max;      // Maximum voltage recorded
    float voltage_min;      // Minimum voltage recorded
    float current_avg;      // Average current
    float current_max;      // Maximum current recorded
    float power_total_mWh;  // Total energy consumed (milliwatt-hours)
    uint32_t sample_count;  // Total samples collected
} power_stats_t;

// ===============================================
// FUNCTION DECLARATIONS
// ===============================================

/**
 * Initialize power monitoring module
 * Configures I2C communication and sets up INA219
 * Should be called once during system startup
 */
void power_init(void);

/**
 * Read bus voltage from INA219
 * @return Voltage in volts (float)
 */
float power_get_voltage(void);

/**
 * Read current from INA219
 * @return Current in milliamps (float)
 */
float power_get_current(void);

/**
 * Read power from INA219
 * @return Power in milliwatts (float)
 */
float power_get_power(void);

/**
 * Get complete power reading with timestamp
 * @param reading Pointer to power_reading_t struct
 * @return true if successful, false if I2C error
 */
bool power_get_reading(power_reading_t *reading);

/**
 * Background task for continuous power monitoring
 * Should be called as a FreeRTOS task
 * @param param Unused parameter (FreeRTOS requirement)
 */
void power_task(void *param);

/**
 * Check if voltage is within safe operating range
 * @return true if voltage is safe, false if out of range
 */
bool power_is_voltage_safe(void);

/**
 * Check if current is within safe operating range
 * @return true if current is safe, false if overcurrent
 */
bool power_is_current_safe(void);

/**
 * Check overall power subsystem health
 * @return true if all power parameters are healthy
 */
bool power_is_healthy(void);

/**
 * Get current power statistics
 * @param stats Pointer to power_stats_t struct
 */
void power_get_statistics(power_stats_t *stats);

/**
 * Reset power statistics counters
 */
void power_reset_statistics(void);

/**
 * Calibrate the INA219 sensor
 * Sets the calibration register for accurate measurements
 * @return true if calibration successful
 */
bool power_calibrate(void);

/**
 * Set custom current calibration value
 * For fine-tuning measurements with specific shunt resistor
 * @param calibration Calibration value
 * @return true if successful
 */
bool power_set_calibration(uint16_t calibration);

/**
 * Check I2C communication with INA219
 * @return true if device responds, false if communication error
 */
bool power_is_connected(void);

/**
 * Get last error code from I2C communication
 * @return Error code (0 = no error)
 */
uint8_t power_get_error(void);

/**
 * Enable/disable power monitoring
 * @param enable true to enable, false to disable
 */
void power_set_enabled(bool enable);

/**
 * Check if power monitoring is enabled
 * @return true if enabled
 */
bool power_is_enabled(void);

#endif // POWER_H