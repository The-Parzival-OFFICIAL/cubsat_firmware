/*
===========================================
TELEMETRY MODULE
-------------------------------------------
Collects and buffers sensor data
Transmits via LoRa if available
Stores locally if LoRa unavailable
===========================================
*/

#include <Arduino.h>
#include <cstring>
#include "telemetry.h"
#include "lora_comm.h"
#include "system_state.h"

extern SemaphoreHandle_t stateMutex;

static telemetry_buffer_t telemetryBuffer = {0};

void telemetry_init(void)
{
    memset(&telemetryBuffer, 0, sizeof(telemetry_buffer_t));
    Serial.println("[TELEMETRY] Telemetry system initialized");
}

void telemetry_add_sample(const telemetry_packet_t *packet)
{
    if (!packet)
        return;

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        // Add to circular buffer
        uint16_t nextHead = (telemetryBuffer.head + 1) % 
                           (TELEMETRY_BUFFER_SIZE / sizeof(telemetry_packet_t));

        if (nextHead == telemetryBuffer.tail)
        {
            // Buffer full - set overflow flag and advance tail
            telemetryBuffer.overflow = true;
            telemetryBuffer.tail = (telemetryBuffer.tail + 1) % 
                                  (TELEMETRY_BUFFER_SIZE / sizeof(telemetry_packet_t));
        }
        else
        {
            telemetryBuffer.count++;
        }

        // Copy packet to buffer
        memcpy(&telemetryBuffer.packets[telemetryBuffer.head], packet, sizeof(telemetry_packet_t));
        telemetryBuffer.head = nextHead;

        xSemaphoreGive(stateMutex);
    }
}

uint16_t telemetry_get_buffered_count(void)
{
    uint16_t count = 0;
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        count = telemetryBuffer.count;
        xSemaphoreGive(stateMutex);
    }
    return count;
}

bool telemetry_read_packet(telemetry_packet_t *packet)
{
    if (!packet)
        return false;

    bool hasData = false;

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        if (telemetryBuffer.count > 0)
        {
            memcpy(packet, &telemetryBuffer.packets[telemetryBuffer.tail], sizeof(telemetry_packet_t));
            telemetryBuffer.tail = (telemetryBuffer.tail + 1) % 
                                  (TELEMETRY_BUFFER_SIZE / sizeof(telemetry_packet_t));
            telemetryBuffer.count--;
            hasData = true;
        }
        xSemaphoreGive(stateMutex);
    }

    return hasData;
}

void telemetry_clear_buffer(void)
{
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)))
    {
        memset(&telemetryBuffer, 0, sizeof(telemetry_buffer_t));
        xSemaphoreGive(stateMutex);
    }
}

void telemetry_serialize_packet(const telemetry_packet_t *packet, uint8_t *buffer)
{
    if (!packet || !buffer)
        return;

    uint16_t offset = 0;

    // Serialize timestamp (4 bytes)
    memcpy(buffer + offset, &packet->timestamp, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Serialize voltage (4 bytes)
    memcpy(buffer + offset, &packet->voltage, sizeof(float));
    offset += sizeof(float);

    // Serialize current (4 bytes)
    memcpy(buffer + offset, &packet->current, sizeof(float));
    offset += sizeof(float);

    // Serialize temperature (4 bytes)
    memcpy(buffer + offset, &packet->temperature, sizeof(float));
    offset += sizeof(float);

    // Serialize accel magnitude (4 bytes)
    memcpy(buffer + offset, &packet->accel_mag, sizeof(float));
    offset += sizeof(float);

    // Serialize gyro magnitude (4 bytes)
    memcpy(buffer + offset, &packet->gyro_mag, sizeof(float));
    offset += sizeof(float);

    // Serialize fault flags and mode (2 bytes)
    buffer[offset++] = packet->fault_flags;
    buffer[offset++] = packet->mode;
}

void telemetry_deserialize_packet(const uint8_t *buffer, telemetry_packet_t *packet)
{
    if (!buffer || !packet)
        return;

    uint16_t offset = 0;

    // Deserialize timestamp
    memcpy(&packet->timestamp, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Deserialize voltage
    memcpy(&packet->voltage, buffer + offset, sizeof(float));
    offset += sizeof(float);

    // Deserialize current
    memcpy(&packet->current, buffer + offset, sizeof(float));
    offset += sizeof(float);

    // Deserialize temperature
    memcpy(&packet->temperature, buffer + offset, sizeof(float));
    offset += sizeof(float);

    // Deserialize accel magnitude
    memcpy(&packet->accel_mag, buffer + offset, sizeof(float));
    offset += sizeof(float);

    // Deserialize gyro magnitude
    memcpy(&packet->gyro_mag, buffer + offset, sizeof(float));
    offset += sizeof(float);

    // Deserialize fault flags and mode
    packet->fault_flags = buffer[offset++];
    packet->mode = buffer[offset++];
}

void telemetry_transmit_if_lora_available(void)
{
    if (!lora_is_available())
    {
        Serial.println("[TELEMETRY] LoRa not available - buffering locally");
        return;
    }

    uint16_t packetCount = telemetry_get_buffered_count();
    if (packetCount == 0)
        return;

    Serial.printf("[TELEMETRY] Transmitting %d buffered packets via LoRa\n", packetCount);

    telemetry_packet_t packet;
    uint8_t serialized[30];  // Size for serialized packet

    while (telemetry_read_packet(&packet))
    {
        telemetry_serialize_packet(&packet, serialized);
        lora_send(serialized, 30);
        vTaskDelay(pdMS_TO_TICKS(100));  // Delay between transmissions
    }

    Serial.println("[TELEMETRY] Transmission complete");
}