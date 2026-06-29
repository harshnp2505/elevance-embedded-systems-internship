# Task 6 — Smart Battery Analytics Engine

## Overview

This project is an ESP32-based intelligent battery monitoring and protection system developed during the Elevance Internship using Wokwi simulation.

The system performs:

* Real-time battery monitoring
* Fault detection
* Runtime state management
* Safety protection
* Cloud telemetry
* Predictive analytics

---

## Technologies Used

* ESP32
* Embedded C/C++
* Arduino Framework
* Wokwi Simulation
* Blynk IoT
* I2C LCD
* Relay Module
* Buzzer

---

## Features

### Monitoring

* 4-cell voltage monitoring
* Average voltage calculation
* Pack voltage calculation

### Safety

* Overvoltage detection
* Sensor disconnect detection
* Cell imbalance detection
* Relay cutoff
* Anti-relay chatter protection

### Analytics

* Battery Health Score
* Failure Probability
* Risk Classification
* AI-inspired Recommendation Engine

### Runtime Modes

* NORMAL
* DEGRADED
* FAILSAFE
* SHUTDOWN

---

## Architecture

Battery Cells → ESP32 → Relay / Buzzer / LCD → Blynk Cloud

---

## Future Improvements

* Temperature sensing
* Current sensing
* SOC estimation
* ML-based predictive analytics
* CAN bus support

---

## Author

Harsh Patel

Embedded Systems | AIoT | Robotics
