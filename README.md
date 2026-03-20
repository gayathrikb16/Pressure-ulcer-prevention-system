# Adaptive Region-Aware Pressure Ulcer Prevention System

## Overview
This project presents a real-time embedded system designed to monitor pressure distribution and reduce the risk of pressure ulcers. It uses Force Sensitive Resistors (FSRs) to detect pressure at different body regions and automatically actuates air cells using solenoid valves for pressure redistribution.

## Objectives
- Monitor pressure levels in real time using FSR sensors  
- Identify high-pressure regions based on a weighted risk approach  
- Automatically trigger pressure relief using solenoid valves  
- Provide a scalable and low-cost embedded solution  

## System Components
- ESP32 Microcontroller  
- Force Sensitive Resistors (FSRs)  
- Solenoid Valves  
- MOSFET Driver Circuit with Flyback Diode  
- External DC Power Supply  

## Working Principle
- Multiple FSR sensors are placed in a region-aware configuration.  
- The ESP32 reads analog values from each sensor.  
- A threshold-based or weighted logic identifies high-pressure zones.  
- Corresponding solenoid valves are activated to redistribute pressure.  

## Features
- Real-time pressure monitoring  
- Automated actuation system  
- Region-based analysis  
- Low-cost and scalable design  

## Pin Configuration (Example)
| Component       | ESP32 Pin |
|-----------------|----------|
| FSR 1           | GPIO 32  |
| FSR 2           | GPIO 33  |
| FSR 3           | GPIO 34  |
| FSR 4           | GPIO 35  |
| FSR 5           | GPIO 36  |
| Solenoid Control| GPIO XX  |

## Technologies Used
- Arduino IDE  
- Embedded C/C++  
- ESP32 Platform  

## Future Improvements
- Calibration to map sensor values to pressure (mmHg)  
- Integration with IoT dashboard for remote monitoring  
- Advanced risk prediction algorithms  
- Medical-grade validation  

## Author
Gayathri K B
