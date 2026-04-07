/*
===========================================
HEALTH MONITORING MODULE
-------------------------------------------
Fault Detection & Self-Healing Logic
Operating Voltage: 3.3V
===========================================
*/

#include <Arduino.h>
#include "health.h"
#include "system_state.h"
#include "power.h"
#include "temperature.h"
#include "esp_task_wdt.h"

// External mutex from main.cpp
extern SemaphoreHandle_t stateMutex;

// Safety thresholds
static safety_thresholds_t safetyLimits = {
    .temp_max = 60.0,           // 60°C
    .temp_min = -40.0,          // -40°C
    .voltage_max = 4.5,         // 4.5V (overvoltage protection)
    .voltage_min = 2.5,         // 2.5V (brownout protection)
    .current_max = 2000.0       // 2000mA (overcurrent)
};

// Recovery state
static recovery_state_t recoveryState = RECOVERY_IDLE;
static uint32_t recoveryAttempts = 0;
static const uint32_t MAX_RECOVERY_ATTEMPTS = 3;

// Fault counters for hysteresis
static uint8_t tempHighCounter = 0;
static uint8_t tempLowCounter = 0;
static uint8_t voltageHighCounter = 0;
static uint8_t voltageLowCounter = 0;
static uint8_t currentHighCounter = 0;

#define FAULT_THRESHOLD 2  // Number of consecutive faults to trigger

void health_init(void)
{
    Serial.println("[HEALTH] Initializing health monitoring system");
    recoveryState = RECOVERY_IDLE;
    recoveryAttempts = 0;
}

uint8_t health_get_faults(void)
{
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        uint8_t faults = systemStatus.fault_flags;
        xSemaphoreGive(stateMutex);
        return faults;
    }
    return 0;
}

void health_clear_fault(fault_flag_t fault)
{
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        systemStatus.fault_flags &= ~fault;
        xSemaphoreGive(stateMutex);
    }
}

recovery_state_t health_get_recovery_state(void)
{
    return recoveryState;
}

void health_set_recovery_state(recovery_state_t state)
{
    recoveryState = state;
}

const char* health_fault_to_string(fault_flag_t fault)
{
    switch (fault)
    {
        case FAULT_TEMP_HIGH:
            return "TEMP_HIGH";
        case FAULT_TEMP_LOW:
            return "TEMP_LOW";
        case FAULT_VOLTAGE_LOW:
            return "VOLTAGE_LOW";
        case FAULT_VOLTAGE_HIGH:
            return "VOLTAGE_HIGH";
        case FAULT_CURRENT_HIGH:
            return "CURRENT_HIGH";
        case FAULT_MPU6050_FAIL:
            return "MPU6050_FAIL";
        case FAULT_LORA_FAIL:
            return "LORA_FAIL";
        case FAULT_I2C_ERROR:
            return "I2C_ERROR";
        default:
            return "UNKNOWN";
    }
}

// ============================================
// TEMPERATURE MONITORING
// ============================================
static void health_check_temperature(float temp)
{
    if (temp > safetyLimits.temp_max)
    {
        tempHighCounter++;
        tempLowCounter = 0;
        
        if (tempHighCounter >= FAULT_THRESHOLD)
        {
            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                systemStatus.fault_flags |= FAULT_TEMP_HIGH;
                xSemaphoreGive(stateMutex);
            }
            Serial.printf("[HEALTH] FAULT: Temperature too high: %.2f°C\n", temp);
        }
    }
    else if (temp < safetyLimits.temp_min)
    {
        tempLowCounter++;
        tempHighCounter = 0;
        
        if (tempLowCounter >= FAULT_THRESHOLD)
        {
            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                systemStatus.fault_flags |= FAULT_TEMP_LOW;
                xSemaphoreGive(stateMutex);
            }
            Serial.printf("[HEALTH] FAULT: Temperature too low: %.2f°C\n", temp);
        }
    }
    else
    {
        // Clear counters when in safe range
        tempHighCounter = 0;
        tempLowCounter = 0;
        health_clear_fault(FAULT_TEMP_HIGH);
        health_clear_fault(FAULT_TEMP_LOW);
    }
}

// ============================================
// VOLTAGE MONITORING
// ============================================
static void health_check_voltage(float voltage)
{
    if (voltage > safetyLimits.voltage_max)
    {
        voltageHighCounter++;
        voltageLowCounter = 0;
        
        if (voltageHighCounter >= FAULT_THRESHOLD)
        {
            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                systemStatus.fault_flags |= FAULT_VOLTAGE_HIGH;
                xSemaphoreGive(stateMutex);
            }
            Serial.printf("[HEALTH] FAULT: Voltage too high: %.2f V\n", voltage);
        }
    }
    else if (voltage < safetyLimits.voltage_min)
    {
        voltageLowCounter++;
        voltageHighCounter = 0;
        
        if (voltageLowCounter >= FAULT_THRESHOLD)
        {
            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                systemStatus.fault_flags |= FAULT_VOLTAGE_LOW;
                xSemaphoreGive(stateMutex);
            }
            Serial.printf("[HEALTH] FAULT: Voltage too low: %.2f V\n", voltage);
        }
    }
    else
    {
        voltageHighCounter = 0;
        voltageLowCounter = 0;
        health_clear_fault(FAULT_VOLTAGE_HIGH);
        health_clear_fault(FAULT_VOLTAGE_LOW);
    }
}

// ============================================
// CURRENT MONITORING
// ============================================
static void health_check_current(float current)
{
    if (current > safetyLimits.current_max)
    {
        currentHighCounter++;
        
        if (currentHighCounter >= FAULT_THRESHOLD)
        {
            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                systemStatus.fault_flags |= FAULT_CURRENT_HIGH;
                xSemaphoreGive(stateMutex);
            }
            Serial.printf("[HEALTH] FAULT: Current too high: %.2f mA\n", current);
        }
    }
    else
    {
        currentHighCounter = 0;
        health_clear_fault(FAULT_CURRENT_HIGH);
    }
}

// ============================================
// AUTOMATIC RECOVERY LOGIC
// ============================================
static void health_attempt_recovery(void)
{
    recoveryState = RECOVERY_IN_PROGRESS;
    recoveryAttempts++;

    Serial.printf("[HEALTH] Recovery attempt %d/%d\n", recoveryAttempts, MAX_RECOVERY_ATTEMPTS);

    // Strategy: Safe mode + system reboot
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        systemStatus.mode = MODE_SAFE;
        xSemaphoreGive(stateMutex);
    }

    // Wait before recovery attempt
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Clear faults to see if they reoccur
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        systemStatus.fault_flags = 0;
        xSemaphoreGive(stateMutex);
    }

    Serial.println("[HEALTH] Entered SAFE MODE - reduced operations");
    recoveryState = RECOVERY_SUCCESS;
    recoveryAttempts = 0;  // Reset after successful recovery
}

static void health_critical_shutdown(void)
{
    Serial.println("[HEALTH] CRITICAL: Recovery failed - entering watchdog reset");
    recoveryState = RECOVERY_FAILED;
    
    // Force watchdog timeout
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void health_check_and_recover(void)
{
    uint8_t faults = health_get_faults();

    if (faults == FAULT_NONE)
    {
        recoveryAttempts = 0;
        if (systemStatus.mode == MODE_SAFE)
        {
            systemStatus.mode = MODE_NORMAL;
            Serial.println("[HEALTH] Recovered - returning to NORMAL mode");
        }
        return;
    }

    // Log detected faults
    Serial.print("[HEALTH] Detected faults: ");
    for (int i = 0; i < 8; i++)
    {
        if (faults & (1 << i))
        {
            Serial.print(health_fault_to_string((fault_flag_t)(1 << i)));
            Serial.print(" ");
        }
    }
    Serial.println();

    // Attempt recovery
    if (recoveryAttempts < MAX_RECOVERY_ATTEMPTS)
    {
        health_attempt_recovery();
    }
    else
    {
        health_critical_shutdown();
    }
}

void health_task(void *param)
{
    esp_task_wdt_add(NULL);

    while (1)
    {
        float temp = temperature_read();
        float voltage = power_get_voltage();
        float current = power_get_current();

        // Check each subsystem
        health_check_temperature(temp);
        health_check_voltage(voltage);
        health_check_current(current);

        // Perform recovery if faults detected
        health_check_and_recover();

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5 seconds
    }
}