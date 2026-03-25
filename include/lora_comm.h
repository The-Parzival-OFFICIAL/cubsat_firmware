#include <cstdint>
#ifndef LORA_COMM_H
#define LORA_COMM_H

// ============================================
// LORA MODULE (SX1278)
// --------------------------------------------
// SPI Pins:
// NSS  -> GPIO5
// RST  -> GPIO14
// DIO0 -> GPIO26
// MOSI -> GPIO23
// MISO -> GPIO19
// SCK  -> GPIO18
// Frequency: 433E6
// ============================================

void lora_init(void);
bool lora_send(const uint8_t *data, uint16_t length);
int  lora_receive(uint8_t *buffer, uint16_t max_len);

#endif