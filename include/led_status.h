#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <cstdint>

// ============================================
// LED STATUS INDICATOR MODULE
// -------------------------------------------
// Red LED (GPIO4)   -> Fault/Error Status
// Green LED (GPIO2) -> System Health Status
// 
// Blinking patterns indicate system state
// ============================================

typedef enum {
    LED_STATUS_OFF = 0,
    LED_STATUS_ON = 1,
    LED_STATUS_BLINK_SLOW = 2,    // 500ms on/off
    LED_STATUS_BLINK_FAST = 3,    // 200ms on/off
    LED_STATUS_BLINK_CRITICAL = 4 // 100ms on/off (SOS pattern)
} led_status_t;

typedef struct {
    uint8_t red_pin;
    uint8_t green_pin;
    bool initialized;
} led_config_t;

// Function declarations
void led_init(uint8_t red_pin, uint8_t green_pin);
void led_set_status(uint8_t led_type, led_status_t status);
void led_update(void);
void led_set_red(led_status_t status);
void led_set_green(led_status_t status);
void led_heartbeat(void);

// LED types
#define LED_RED 0
#define LED_GREEN 1

#endif