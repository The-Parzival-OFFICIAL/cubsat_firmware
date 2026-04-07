#ifndef LORA_COMM_OPTIONAL_H
#define LORA_COMM_OPTIONAL_H

#include <cstdint>

// ============================================
// LORA MODULE - OPTIONAL DETECTION
// SX1278 @ 433 MHz
// -------------------------------------------
// SPI CONNECTIONS:
// NSS  -> GPIO5
// RST  -> GPIO14
// DIO0 -> GPIO26
// MOSI -> GPIO23
// MISO -> GPIO19
// SCK  -> GPIO18
// ============================================

typedef enum {
    LORA_STATE_UNKNOWN = 0,
    LORA_STATE_NOT_DETECTED = 1,
    LORA_STATE_DETECTED = 2,
    LORA_STATE_INITIALIZED = 3,
    LORA_STATE_ERROR = 4
} lora_state_t;

typedef struct {
    lora_state_t state;
    bool is_available;
    uint32_t detection_time;
    uint32_t last_rx_time;
} lora_status_t;

// Function declarations
void lora_init_optional(void);
lora_status_t lora_get_status(void);
bool lora_is_available(void);
bool lora_detect(void);
bool lora_send(const uint8_t *data, uint16_t length);
int lora_receive(uint8_t *buffer, uint16_t max_len);

#endif