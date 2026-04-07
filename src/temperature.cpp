/*
===========================================
TEMPERATURE SENSOR MODULE - LM35DZ
-------------------------------------------
Analog Temperature Sensor
Output: 10mV/°C at pin GPIO36 (ADC)
Temperature Range: -40°C to 125°C
===========================================
*/

#include <Arduino.h>
#include "temperature.h"

#define TEMP_SENSOR_PIN GPIO_NUM_36  // ADC0
#define TEMP_SAMPLES 10

static temp_sensor_type_t sensorType = TEMP_SENSOR_NONE;
static float lastTemp = 0.0;
static bool sensorInitialized = false;

void temperature_init(temp_sensor_type_t sensor_type, uint8_t pin)
{
    sensorType = sensor_type;
    sensorInitialized = true;

    if (sensorType == TEMP_SENSOR_LM35DZ)
    {
        analogSetPinAttenuation(TEMP_SENSOR_PIN, ADC_11db);
        Serial.println("[TEMP] LM35DZ initialized on GPIO36");
    }
    else if (sensorType == TEMP_SENSOR_DS18B20)
    {
        Serial.println("[TEMP] DS18B20 mode selected (requires OneWire library)");
    }
}

float temperature_read(void)
{
    if (!sensorInitialized)
        return 0.0;

    if (sensorType == TEMP_SENSOR_LM35DZ)
    {
        // Read multiple samples and average
        uint32_t sumADC = 0;
        for (int i = 0; i < TEMP_SAMPLES; i++)
        {
            sumADC += analogRead(TEMP_SENSOR_PIN);
            delayMicroseconds(100);
        }

        uint32_t avgADC = sumADC / TEMP_SAMPLES;

        // Convert ADC value to voltage (12-bit ADC, 3.3V reference)
        // ADC value 0-4095 maps to 0-3.3V
        float voltage = (avgADC / 4095.0) * 3.3;

        // LM35DZ: 10mV per degree Celsius
        // For better accuracy, we account for the sensor's output characteristics
        float tempC = voltage * 100.0;  // 10mV/°C = 100mV/10°C

        // Apply calibration offset if needed
        lastTemp = tempC;
        return tempC;
    }

    return lastTemp;
}

bool temperature_is_valid(void)
{
    return sensorInitialized;
}

float temperature_get_last(void)
{
    return lastTemp;
}