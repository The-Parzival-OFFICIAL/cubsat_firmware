/*
===========================================
LORA MODULE - SX1278
-------------------------------------------
SPI CONNECTIONS:
NSS  -> GPIO5
RST  -> GPIO14
DIO0 -> GPIO26
MOSI -> GPIO23
MISO -> GPIO19
SCK  -> GPIO18

Frequency: 433 MHz
===========================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "lora_comm.h"

#define LORA_NSS   5
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  433E6

void lora_init(void)
{
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(LORA_FREQ))
    {
        Serial.println("ERROR: LoRa init failed!");
        while (1);
    }

    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setTxPower(17);

    Serial.println("LoRa initialized");
}

bool lora_send(const uint8_t *data, uint16_t length)
{
    LoRa.beginPacket();
    LoRa.write(data, length);
    LoRa.endPacket();

    return true;
}

int lora_receive(uint8_t *buffer, uint16_t max_len)
{
    int packetSize = LoRa.parsePacket();
    if (!packetSize)
        return 0;

    int index = 0;
    while (LoRa.available() && index < max_len)
    {
        buffer[index++] = LoRa.read();
    }

    return index;
}