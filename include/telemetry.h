#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <cstdint>
#include <cstring>

// ============================================
// TELEMETRY MODULE
// Data collection, logging, and transmission
// ============================================

#define TELEMETRY_BUFFER_SIZE 512
#define TELEMETRY_PACKET_SIZE 64

typedef struct {
    uint32_t timestamp;
    float voltage;
    float current;
    float temperature;
    float accel_mag;
    float gyro_mag;
    uint8_t fault_flags;
    uint8_t mode;
} telemetry_packet_t;

typedef struct {
    telemetry_packet_t packets[TELEMETRY_BUFFER_SIZE / sizeof(telemetry_packet_t)];
    uint16_t head;      // Write pointer
    uint16_t tail;      // Read pointer
    uint16_t count;     // Number of valid packets
    bool overflow;      // Buffer overflow flag
} telemetry_buffer_t;

// Function declarations
void telemetry_init(void);
void telemetry_add_sample(const telemetry_packet_t *packet);
uint16_t telemetry_get_buffered_count(void);
bool telemetry_read_packet(telemetry_packet_t *packet);
void telemetry_clear_buffer(void);
void telemetry_transmit_if_lora_available(void);
void telemetry_serialize_packet(const telemetry_packet_t *packet, uint8_t *buffer);
void telemetry_deserialize_packet(const uint8_t *buffer, telemetry_packet_t *packet);

#endif