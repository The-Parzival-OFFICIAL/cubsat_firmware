#ifndef POWER_H
#define POWER_H

// ===============================
// POWER MODULE (INA219)
// I2C Pins:
// SDA -> GPIO21
// SCL -> GPIO22
// ===============================

void power_init(void);
void power_task(void *param);

float power_get_voltage(void);
float power_get_current(void);
float power_get_power(void);

#endif