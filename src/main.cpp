/*
===========================================
CUBESAT ON-BOARD COMPUTER (OBC)
-------------------------------------------
ESP32 DevKit V1
Main controller with adaptive self-healing

SYSTEM ARCHITECTURE:
- Health Monitoring Task
- Power Management Task
- Sensor Fusion Task
- Telemetry Task
- Communication Task (LoRa optional)
- Watchdog Task

I2C: SDA=GPIO21, SCL=GPIO22
Watchdog Timeout: 8 seconds
===========================================
*/

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

// All module headers
#include "system_state.h"
#include "health.h"
#include "power.h"
#include "temperature.h"
#include "imu.h"
#include "led_status.h"
#include "lora_comm.h"
#include "telemetry.h"

// ============================================
// GLOBAL VARIABLES
// ============================================
SemaphoreHandle_t stateMutex;
uint32_t systemStartTime = 0;

// ============================================
// SYSTEM CONFIGURATION
// ============================================
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY_HIGH 3
#define TASK_PRIORITY_NORMAL 2
#define TASK_PRIORITY_LOW 1

// ============================================
// TASK: POWER MONITORING
// ============================================
void power_monitoring_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] Power monitoring started");

    while (1)
    {
        float voltage = power_get_voltage();
        float current = power_get_current();
        float power = power_get_power();

        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
        {
            systemStatus.voltage = voltage;
            systemStatus.current = current;
            systemStatus.power = power;
            xSemaphoreGive(stateMutex);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));  // Every 2 seconds
    }
}

// ============================================
// TASK: SENSOR FUSION (Temperature & IMU)
// ============================================
void sensor_fusion_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] Sensor fusion started");

    imu_data_t imuData;

    while (1)
    {
        // Read temperature
        float temp = temperature_read();

        // Read IMU
        bool imuSuccess = imu_read(&imuData);

        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
        {
            systemStatus.temperature = temp;

            if (imuSuccess)
            {
                systemStatus.accel_x = imuData.ax;
                systemStatus.accel_y = imuData.ay;
                systemStatus.accel_z = imuData.az;
                systemStatus.gyro_x = imuData.gx;
                systemStatus.gyro_y = imuData.gy;
                systemStatus.gyro_z = imuData.gz;
            }
            else
            {
                systemStatus.fault_flags |= FAULT_IMU_FAIL;
            }

            xSemaphoreGive(stateMutex);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Every 1 second
    }
}

// ============================================
// TASK: HEALTH MONITORING & FAULT RECOVERY
// ============================================
void health_monitoring_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] Health monitoring started");

    uint32_t lastCheck = 0;

    while (1)
    {
        uint32_t currentTime = millis();

        // Perform health check every 5 seconds
        if (currentTime - lastCheck > 5000)
        {
            // Get current sensor readings
            float temp = 0, voltage = 0, current = 0;

            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                temp = systemStatus.temperature;
                voltage = systemStatus.voltage;
                current = systemStatus.current;
                xSemaphoreGive(stateMutex);
            }

            // Perform health checks
            health_check_and_recover();

            // Update system mode based on faults
            uint8_t faults = health_get_faults();

            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
            {
                if (faults == 0)
                {
                    systemStatus.mode = MODE_NORMAL;
                    systemStatus.cycles_normal++;
                }
                else if (faults & (FAULT_TEMP_HIGH | FAULT_VOLTAGE_LOW | FAULT_CURRENT_HIGH))
                {
                    systemStatus.mode = MODE_SAFE;
                    systemStatus.cycles_safe++;
                }
                else
                {
                    systemStatus.mode = MODE_CRITICAL;
                    systemStatus.cycles_critical++;
                }

                xSemaphoreGive(stateMutex);
            }

            lastCheck = currentTime;
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================
// TASK: LED STATUS INDICATION
// ============================================
void led_status_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] LED status indication started");

    while (1)
    {
        uint8_t faults = 0;
        system_mode_t mode = MODE_NORMAL;

        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
        {
            faults = systemStatus.fault_flags;
            mode = systemStatus.mode;
            xSemaphoreGive(stateMutex);
        }

        // Update LED status based on system state
        if (faults == 0)
        {
            led_set_red(LED_STATUS_OFF);
        }
        else if (mode == MODE_SAFE)
        {
            led_set_red(LED_STATUS_BLINK_SLOW);
        }
        else if (mode == MODE_CRITICAL)
        {
            led_set_red(LED_STATUS_BLINK_CRITICAL);
        }

        // Green LED shows system health
        switch (mode)
        {
            case MODE_NORMAL:
                led_set_green(LED_STATUS_ON);
                break;
            case MODE_SAFE:
                led_set_green(LED_STATUS_BLINK_SLOW);
                break;
            case MODE_CRITICAL:
                led_set_green(LED_STATUS_BLINK_FAST);
                break;
        }

        // Update LEDs
        led_update();

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));  // Update every 100ms
    }
}

// ============================================
// TASK: TELEMETRY COLLECTION
// ============================================
void telemetry_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] Telemetry started");

    while (1)
    {
        telemetry_packet_t packet = {0};

        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
        {
            packet.timestamp = millis();
            packet.voltage = systemStatus.voltage;
            packet.current = systemStatus.current;
            packet.temperature = systemStatus.temperature;

            // Calculate magnitudes
            packet.accel_mag = sqrt(systemStatus.accel_x * systemStatus.accel_x +
                                   systemStatus.accel_y * systemStatus.accel_y +
                                   systemStatus.accel_z * systemStatus.accel_z);

            packet.gyro_mag = sqrt(systemStatus.gyro_x * systemStatus.gyro_x +
                                  systemStatus.gyro_y * systemStatus.gyro_y +
                                  systemStatus.gyro_z * systemStatus.gyro_z);

            packet.fault_flags = systemStatus.fault_flags;
            packet.mode = systemStatus.mode;

            xSemaphoreGive(stateMutex);
        }

        // Add to telemetry buffer
        telemetry_add_sample(&packet);

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));  // Every 5 seconds
    }
}

// ============================================
// TASK: COMMUNICATION
// ============================================
void communication_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    Serial.println("[TASK] Communication task started");

    uint32_t lastTransmit = 0;

    while (1)
    {
        uint32_t currentTime = millis();

        // Attempt to transmit every 30 seconds
        if (currentTime - lastTransmit > 30000)
        {
            if (lora_is_available())
            {
                Serial.println("[COMM] LoRa available - attempting transmission");
                telemetry_transmit_if_lora_available();
            }
            else
            {
                uint16_t bufferedCount = telemetry_get_buffered_count();
                Serial.printf("[COMM] LoRa unavailable - %d packets buffered locally\n", bufferedCount);
            }

            lastTransmit = currentTime;
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5 seconds
    }
}

// ============================================
// TASK: SYSTEM HEARTBEAT & DIAGNOSTICS
// ============================================
void heartbeat_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);

    while (1)
    {
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
        {
            uint32_t uptime = (millis() - systemStartTime) / 1000;
            systemStatus.uptime_seconds = uptime;

            Serial.println("\n========== SYSTEM STATUS ==========");
            Serial.printf("Uptime: %lu seconds\n", uptime);
            Serial.printf("Mode: %s\n", 
                systemStatus.mode == MODE_NORMAL ? "NORMAL" :
                systemStatus.mode == MODE_SAFE ? "SAFE" : "CRITICAL");
            Serial.printf("Voltage: %.2f V | Current: %.2f mA\n", 
                systemStatus.voltage, systemStatus.current);
            Serial.printf("Temperature: %.2f °C\n", systemStatus.temperature);
            Serial.printf("Accel: (%.2f, %.2f, %.2f) g\n", 
                systemStatus.accel_x, systemStatus.accel_y, systemStatus.accel_z);
            Serial.printf("Faults: 0x%02X | LoRa: %s\n", 
                systemStatus.fault_flags, 
                lora_is_available() ? "AVAILABLE" : "UNAVAILABLE");
            Serial.printf("Mode cycles - Normal:%lu Safe:%lu Critical:%lu\n",
                systemStatus.cycles_normal,
                systemStatus.cycles_safe,
                systemStatus.cycles_critical);
            Serial.println("===================================\n");

            led_heartbeat();  // Quick LED pulse
            xSemaphoreGive(stateMutex);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(30000));  // Every 30 seconds
    }
}

// ============================================
// SETUP FUNCTION
// ============================================
void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n========================================");
    Serial.println("  CUBESAT ON-BOARD COMPUTER (OBC)");
    Serial.println("  Adaptive Self-Healing System");
    Serial.println("========================================\n");

    systemStartTime = millis();

    // Initialize watchdog timer (8 second timeout)
    esp_task_wdt_init(8, true);
    esp_task_wdt_add(NULL);

    // Create system mutex
    stateMutex = xSemaphoreCreateMutex();
    if (!stateMutex)
    {
        Serial.println("ERROR: Failed to create mutex!");
        while (1);
    }

    // Initialize all subsystems
    Serial.println("\n[SETUP] Initializing subsystems...");

    power_init();
    temperature_init(TEMP_SENSOR_LM35DZ, GPIO_NUM_36);
    imu_init();
    led_init(4, 2);  // Red LED on GPIO4, Green on GPIO2
    health_init();
    telemetry_init();
    lora_init_optional();

    Serial.println("[SETUP] All subsystems initialized\n");

    // Create FreeRTOS tasks
    Serial.println("[SETUP] Creating FreeRTOS tasks...");

    xTaskCreate(power_monitoring_task, "PowerMonitor", TASK_STACK_SIZE, NULL, TASK_PRIORITY_NORMAL, NULL);
    xTaskCreate(sensor_fusion_task, "SensorFusion", TASK_STACK_SIZE, NULL, TASK_PRIORITY_NORMAL, NULL);
    xTaskCreate(health_monitoring_task, "HealthMonitor", TASK_STACK_SIZE, NULL, TASK_PRIORITY_HIGH, NULL);
    xTaskCreate(led_status_task, "LEDStatus", TASK_STACK_SIZE, NULL, TASK_PRIORITY_LOW, NULL);
    xTaskCreate(telemetry_task, "Telemetry", TASK_STACK_SIZE, NULL, TASK_PRIORITY_NORMAL, NULL);
    xTaskCreate(communication_task, "Communication", TASK_STACK_SIZE, NULL, TASK_PRIORITY_LOW, NULL);
    xTaskCreate(heartbeat_task, "Heartbeat", TASK_STACK_SIZE, NULL, TASK_PRIORITY_LOW, NULL);

    Serial.println("[SETUP] All tasks created - System ready!\n");
}

// ============================================
// LOOP FUNCTION (Not used with FreeRTOS)
// ============================================
void loop()
{
    // FreeRTOS scheduler handles all tasks
    // This function is kept but not used
    vTaskDelay(pdMS_TO_TICKS(1000));
}