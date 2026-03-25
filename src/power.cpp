/*
===========================================
POWER MODULE - INA219
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

Adafruit_INA219 ina219;

// Mutex defined in main.cpp
extern SemaphoreHandle_t stateMutex;

void power_init(void)
{
    // Initialize I2C on defined pins
    Wire.begin(21, 22);

    if (!ina219.begin())
    {
        Serial.println("ERROR: INA219 not detected!");
        while (1);
    }

    // Configure measurement range
    ina219.setCalibration_32V_2A();
}

float power_get_voltage(void)
{
    return ina219.getBusVoltage_V();
}

float power_get_current(void)
{
    return ina219.getCurrent_mA();
}

float power_get_power(void)
{
    return ina219.getPower_mW();
}

void power_task(void *param)
{
    esp_task_wdt_add(NULL);

    while (1)
    {
        float voltage = power_get_voltage();
        float current = power_get_current();

        if (xSemaphoreTake(stateMutex, portMAX_DELAY))
        {
            systemStatus.voltage = voltage;
            systemStatus.current = current;
            xSemaphoreGive(stateMutex);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}