/*
===========================================
POWER MODULE - INA219 POWER MONITOR
-------------------------------------------
I2C CONNECTIONS:
SDA  -> GPIO21
SCL  -> GPIO22

INA219 POWER PATH:
Battery +  -> VIN+
VIN-       -> System Load +
GND        -> Common GND

Operating Voltage: 3.3V
Calibration: 32V / 2A Mode
===========================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

#include "power.h"
#include "system_state.h"
#include "esp_task_wdt.h"

// Mutex defined in main.cpp
extern SemaphoreHandle_t stateMutex;

// ===============================================
// STATIC VARIABLES
// ===============================================
static Adafruit_INA219 ina219;
static bool powerInitialized = false;
static bool powerEnabled = true;
static uint8_t lastError = 0;

// Statistics tracking
static power_stats_t powerStats = {
    .voltage_avg = 0.0,
    .voltage_max = 0.0,
    .voltage_min = 5.0,  // Start high so first reading updates it
    .current_avg = 0.0,
    .current_max = 0.0,
    .power_total_mWh = 0.0,
    .sample_count = 0
};

// Circular buffer for averaging
static float voltageBuffer[POWER_SAMPLE_COUNT] = {0};
static float currentBuffer[POWER_SAMPLE_COUNT] = {0};
static uint8_t bufferIndex = 0;
static uint32_t lastSampleTime = 0;

// ===============================================
// INITIALIZATION
// ===============================================
void power_init(void)
{
    Serial.println("[POWER] Initializing INA219 power monitor...");

    // Initialize I2C on defined pins
    Wire.begin(21, 22);
    Wire.setClock(400000);  // 400 kHz I2C frequency

    // Check if INA219 is detected
    if (!ina219.begin())
    {
        Serial.println("[POWER] ERROR: INA219 not detected!");
        Serial.println("[POWER] Check I2C wiring: SDA=GPIO21, SCL=GPIO22");
        powerInitialized = false;
        lastError = 1;
        return;
    }

    // Configure measurement range
    // Using 32V / 2A mode for maximum range
    ina219.setCalibration_32V_2A();

    powerInitialized = true;
    powerEnabled = true;
    lastError = 0;

    Serial.println("[POWER] INA219 initialized successfully");
    Serial.println("[POWER] Range: 0-32V / 0-2A");
    Serial.println("[POWER] Resolution: 16-bit");
}

// ===============================================
// BASIC READINGS
// ===============================================
float power_get_voltage(void)
{
    if (!powerInitialized || !powerEnabled)
        return 0.0;

    try
    {
        float voltage = ina219.getBusVoltage_V();
        return voltage;
    }
    catch (...)
    {
        lastError = 2;
        return 0.0;
    }
}

float power_get_current(void)
{
    if (!powerInitialized || !powerEnabled)
        return 0.0;

    try
    {
        float current = ina219.getCurrent_mA();
        return current;
    }
    catch (...)
    {
        lastError = 3;
        return 0.0;
    }
}

float power_get_power(void)
{
    if (!powerInitialized || !powerEnabled)
        return 0.0;

    try
    {
        float power = ina219.getPower_mW();
        return power;
    }
    catch (...)
    {
        lastError = 4;
        return 0.0;
    }
}

// ===============================================
// COMPLETE POWER READING
// ===============================================
bool power_get_reading(power_reading_t *reading)
{
    if (!reading || !powerInitialized || !powerEnabled)
        return false;

    try
    {
        reading->bus_voltage = ina219.getBusVoltage_V();
        reading->shunt_voltage = ina219.getShuntVoltage_mV();
        reading->current_mA = ina219.getCurrent_mA();
        reading->power_mW = ina219.getPower_mW();
        reading->timestamp_ms = millis();
        reading->is_valid = true;
        lastError = 0;
        return true;
    }
    catch (...)
    {
        reading->is_valid = false;
        lastError = 5;
        return false;
    }
}

// ===============================================
// SAFETY CHECKS
// ===============================================
bool power_is_voltage_safe(void)
{
    if (!powerInitialized)
        return false;

    float voltage = power_get_voltage();
    return (voltage >= POWER_VOLTAGE_MIN && voltage <= POWER_VOLTAGE_MAX);
}

bool power_is_current_safe(void)
{
    if (!powerInitialized)
        return false;

    float current = power_get_current();
    return (current <= POWER_CURRENT_MAX);
}

bool power_is_healthy(void)
{
    return power_is_voltage_safe() && power_is_current_safe();
}

// ===============================================
// STATISTICS
// ===============================================
void power_get_statistics(power_stats_t *stats)
{
    if (!stats)
        return;

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        *stats = powerStats;
        xSemaphoreGive(stateMutex);
    }
}

void power_reset_statistics(void)
{
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        powerStats.voltage_avg = 0.0;
        powerStats.voltage_max = 0.0;
        powerStats.voltage_min = 5.0;
        powerStats.current_avg = 0.0;
        powerStats.current_max = 0.0;
        powerStats.power_total_mWh = 0.0;
        powerStats.sample_count = 0;
        xSemaphoreGive(stateMutex);
    }
}

static void power_update_statistics(float voltage, float current, float power)
{
    if (!xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
        return;

    // Update voltage statistics
    if (powerStats.sample_count == 0)
    {
        powerStats.voltage_min = voltage;
        powerStats.voltage_max = voltage;
    }
    else
    {
        if (voltage > powerStats.voltage_max)
            powerStats.voltage_max = voltage;
        if (voltage < powerStats.voltage_min)
            powerStats.voltage_min = voltage;
    }

    // Update current maximum
    if (current > powerStats.current_max)
        powerStats.current_max = current;

    // Update running averages
    float samples = powerStats.sample_count + 1;
    powerStats.voltage_avg = (powerStats.voltage_avg * powerStats.sample_count + voltage) / samples;
    powerStats.current_avg = (powerStats.current_avg * powerStats.sample_count + current) / samples;

    // Update energy consumption (mWh)
    // Energy = Power × Time
    // Assuming measurements every ~2 seconds
    powerStats.power_total_mWh += (power / 1000.0) * (2.0 / 3600.0);  // Convert to mWh

    powerStats.sample_count++;

    xSemaphoreGive(stateMutex);
}

// ===============================================
// CALIBRATION
// ===============================================
bool power_calibrate(void)
{
    if (!powerInitialized)
        return false;

    try
    {
        ina219.setCalibration_32V_2A();
        lastError = 0;
        return true;
    }
    catch (...)
    {
        lastError = 6;
        return false;
    }
}

bool power_set_calibration(uint16_t calibration)
{
    if (!powerInitialized)
        return false;

    try
    {
        // Write to calibration register
        Wire.beginTransmission(INA219_I2C_ADDR);
        Wire.write(INA219_REG_CALIBRATION);
        Wire.write((calibration >> 8) & 0xFF);
        Wire.write(calibration & 0xFF);
        Wire.endTransmission();
        
        lastError = 0;
        return true;
    }
    catch (...)
    {
        lastError = 7;
        return false;
    }
}

// ===============================================
// STATUS CHECKS
// ===============================================
bool power_is_connected(void)
{
    if (!powerInitialized)
        return false;

    try
    {
        // Try to read version register
        Wire.beginTransmission(INA219_I2C_ADDR);
        Wire.write(0xFE);  // DIE_ID register
        Wire.endTransmission(false);
        
        Wire.requestFrom(INA219_I2C_ADDR, 2);
        if (Wire.available() >= 2)
        {
            Wire.read();  // Read both bytes
            Wire.read();
            lastError = 0;
            return true;
        }
        lastError = 8;
        return false;
    }
    catch (...)
    {
        lastError = 9;
        return false;
    }
}

uint8_t power_get_error(void)
{
    return lastError;
}

void power_set_enabled(bool enable)
{
    powerEnabled = enable;
    if (enable)
        Serial.println("[POWER] Power monitoring ENABLED");
    else
        Serial.println("[POWER] Power monitoring DISABLED");
}

bool power_is_enabled(void)
{
    return powerEnabled;
}

// ===============================================
// BACKGROUND TASK
// ===============================================
void power_task(void *param)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] Power monitoring task started");

    uint32_t cycleCount = 0;

    while (1)
    {
        if (powerEnabled && powerInitialized)
        {
            float voltage = power_get_voltage();
            float current = power_get_current();
            float power = power_get_power();

            // Update statistics
            power_update_statistics(voltage, current, power);

            // Update global system state
            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                systemStatus.voltage = voltage;
                systemStatus.current = current;
                systemStatus.power = power;
                xSemaphoreGive(stateMutex);
            }

            // Periodic debug output every 30 seconds (15 cycles × 2 sec)
            cycleCount++;
            if (cycleCount % 15 == 0)
            {
                Serial.printf("[POWER] V=%.2fV | I=%.1fmA | P=%.1fmW | Health=%s\n",
                    voltage, current, power,
                    power_is_healthy() ? "OK" : "FAULT");
            }
        }
        else if (!powerInitialized)
        {
            Serial.println("[POWER] Warning: Power module not initialized");
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));  // Sample every 2 seconds
    }
}