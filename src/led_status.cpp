/*
===========================================
LED STATUS MODULE
-------------------------------------------
GPIO4  -> Red LED (via 220Ω resistor)
GPIO2  -> Green LED (via 220Ω resistor)

LED Patterns:
- OFF: System failure
- ON: Steady state
- BLINK_SLOW: Warning/reduced mode
- BLINK_FAST: Active issue
- BLINK_CRITICAL: Critical fault
===========================================
*/

#include <Arduino.h>
#include "led_status.h"

#define RED_LED_PIN 4
#define GREEN_LED_PIN 2

static led_status_t redStatus = LED_STATUS_OFF;
static led_status_t greenStatus = LED_STATUS_OFF;
static uint32_t lastBlinkTime = 0;
static bool ledState = false;

void led_init(uint8_t red_pin, uint8_t green_pin)
{
    pinMode(red_pin, OUTPUT);
    pinMode(green_pin, OUTPUT);
    digitalWrite(red_pin, LOW);
    digitalWrite(green_pin, LOW);

    Serial.printf("[LED] Initialized: Red=GPIO%d, Green=GPIO%d\n", red_pin, green_pin);
}

void led_set_red(led_status_t status)
{
    redStatus = status;
}

void led_set_green(led_status_t status)
{
    greenStatus = status;
}

void led_set_status(uint8_t led_type, led_status_t status)
{
    if (led_type == LED_RED)
        led_set_red(status);
    else if (led_type == LED_GREEN)
        led_set_green(status);
}

// Helper function to update a single LED
static void update_led(uint8_t pin, led_status_t status)
{
    uint32_t currentTime = millis();
    uint32_t interval = 0;

    switch (status)
    {
        case LED_STATUS_OFF:
            digitalWrite(pin, LOW);
            break;

        case LED_STATUS_ON:
            digitalWrite(pin, HIGH);
            break;

        case LED_STATUS_BLINK_SLOW:
            interval = 500;  // 500ms on/off
            if ((currentTime / interval) % 2 == 0)
                digitalWrite(pin, HIGH);
            else
                digitalWrite(pin, LOW);
            break;

        case LED_STATUS_BLINK_FAST:
            interval = 200;  // 200ms on/off
            if ((currentTime / interval) % 2 == 0)
                digitalWrite(pin, HIGH);
            else
                digitalWrite(pin, LOW);
            break;

        case LED_STATUS_BLINK_CRITICAL:
            // SOS pattern: short, short, short, long, long, long, short, short, short
            interval = 100;  // 100ms units
            uint32_t phase = (currentTime / interval) % 30;
            // SOS = 3 short (on for 1, off for 1), 3 long (on for 3, off for 1), 3 short
            if ((phase >= 0 && phase < 1) ||    // S
                (phase >= 2 && phase < 3) ||    // S
                (phase >= 4 && phase < 5) ||    // S
                (phase >= 6 && phase < 9) ||    // O (long)
                (phase >= 10 && phase < 13) ||  // O (long)
                (phase >= 14 && phase < 17) ||  // O (long)
                (phase >= 18 && phase < 19) ||  // S
                (phase >= 20 && phase < 21) ||  // S
                (phase >= 22 && phase < 23))    // S
            {
                digitalWrite(pin, HIGH);
            }
            else
            {
                digitalWrite(pin, LOW);
            }
            break;

        default:
            digitalWrite(pin, LOW);
            break;
    }
}

void led_update(void)
{
    update_led(RED_LED_PIN, redStatus);
    update_led(GREEN_LED_PIN, greenStatus);
}

void led_heartbeat(void)
{
    // Quick pulse to indicate system is alive
    digitalWrite(GREEN_LED_PIN, HIGH);
    delayMicroseconds(50);
    digitalWrite(GREEN_LED_PIN, LOW);
}