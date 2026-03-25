/*
===========================================
SATELLITE MAIN FILE
-------------------------------------------
ESP32 DevKit V1

I2C:
SDA -> GPIO21
SCL -> GPIO22

Watchdog Timeout: 8 seconds
===========================================
*/

#include <Arduino.h>
#include "esp_task_wdt.h"

#include "system_state.h"
#include "power.h"

SemaphoreHandle_t stateMutex;

void heartbeatTask(void *pvParameters)
{
    esp_task_wdt_add(NULL);

    while (1)
    {
        if (xSemaphoreTake(stateMutex, portMAX_DELAY))
        {
            Serial.print("Voltage: ");
            Serial.print(systemStatus.voltage);
            Serial.print(" V  Current: ");
            Serial.print(systemStatus.current);
            Serial.println(" mA");

            xSemaphoreGive(stateMutex);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void setup()
{
    Serial.begin(115200);

    esp_task_wdt_init(8, true);

    stateMutex = xSemaphoreCreateMutex();

    power_init();

    xTaskCreate(power_task, "PowerTask", 4096, NULL, 2, NULL);
    xTaskCreate(heartbeatTask, "HeartbeatTask", 4096, NULL, 1, NULL);
}

void loop()
{
}