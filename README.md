# CubeSat On-Board Computer (OBC) Prototype 🚀

**ESP32-based embedded system simulating CubeSat subsystems with real-time monitoring and SAFE MODE capability.**

---

## Overview

This project presents a **compact CubeSat OBC** using ESP32, integrating:

- Power monitoring (EPS)  
- Thermal sensing  
- Motion/orientation detection (ADCS)  

**FreeRTOS** enables real-time multitasking for sensor data acquisition and system reliability.  
The system is **modular, scalable**, and simulates fault-aware SAFE MODE operation.

---

## Objectives

- Build a CubeSat-like OBC using ESP32  
- Monitor power via INA219  
- Measure temperature via LM35DZ  
- Capture motion/orientation via MPU6050  
- Implement multitasking using FreeRTOS  
- Develop a modular architecture  
- Simulate fault detection and SAFE MODE behavior

---

## Hardware Components

- **ESP32 DevKit V1** – main controller  
- **INA219** – voltage & current sensor  
- **LM35DZ** – temperature sensor  
- **MPU6050** – IMU for motion & orientation  
- Breadboard, jumper wires, USB power  

> Communication modules like LoRa/Bluetooth not implemented yet

---

## System Architecture


Battery → INA219 → ESP32 → Sensors → Serial Output
↓
FreeRTOS Task Manager


**Subsystems:**

- EPS → INA219  
- Thermal → LM35DZ  
- ADCS → MPU6050  
- Controller → ESP32  

---

## Methodology

1. Initialize sensors via I2C/ADC  
2. Create independent FreeRTOS tasks for each sensor  
3. Poll sensors periodically and update shared system state  
4. Output telemetry via serial monitor  
5. Use mutexes for shared data protection  
6. Use watchdog timer for system reliability  

---

## System Flow

Start → Initialize Sensors → Create FreeRTOS Tasks
      → Read Sensor Data (Parallel) → Update System State
      → Display Telemetry → Repeat
---
## Novelty & Adaptive Features

- **SAFE MODE simulation** for abnormal conditions (e.g., low voltage or high temperature)  
- **Modular design** allows adding new sensors or machine learning modules  
- Demonstrates **autonomy similar to real CubeSat systems**  

---

## Results

- Continuous **real-time monitoring** of voltage, current, temperature, and orientation  
- **Stable multitasking** using FreeRTOS  
- **Reliable prototype operation** validated  
- Demonstrates **feasibility for CubeSat-like educational/research platforms**  

---

## Applications

- CubeSat & nanosatellite development  
- Space research prototypes  
- Embedded system education  
- IoT-based monitoring systems  
- Autonomous systems development  

---

## Limitations

- No communication module implemented yet (e.g., LoRa/Bluetooth)  
- Low-cost sensors limit accuracy  
- Advanced fault detection not implemented  
- Power system not optimized for real space conditions  
- No onboard data logging or storage  

---

## Future Enhancements

- Add communication module (LoRa/Bluetooth)  
- Implement advanced fault detection and SAFE MODE automation  
- Optimize power system for real missions  
- Integrate ML-based predictive analysis  
- Add onboard storage & telemetry logging  

---

## References

1. ESP32 Technical Reference Manual  
2. INA219 Current Sensor Datasheet  
3. LM35DZ Temperature Sensor Datasheet  
4. MPU6050 IMU Datasheet  
5. CubeSat literature & embedded system references
