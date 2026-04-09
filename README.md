# Smart Health Monitoring System for Subterranean Workers

## Overview
A LoRa-based real-time monitoring system designed to improve worker safety in underground environments such as mines, tunnels, and confined industrial spaces.

This project continuously monitors both physiological and environmental conditions using an ESP32-based wearable transmitter node and a receiver base station.

The system detects hazardous gas exposure, abnormal vital signs, and unsafe environmental conditions, then wirelessly transmits alerts over long-range LoRa communication.

---

## Hardware Used

### Transmitter Node
- ESP32
- MQ-7 (Carbon Monoxide Sensor)
- MQ-4 (Methane Gas Sensor)
- DHT22 (Temperature & Humidity Sensor)
- MAX30102 (Heart Rate + SpO₂ Sensor)
- SX1278 LoRa Module

### Receiver Node
- ESP32
- SX1278 LoRa Module
- LCD Display
- Buzzer Alert Unit

---

## Features
- Real-time gas hazard monitoring
- Heart rate and SpO₂ tracking
- Temperature and humidity sensing
- Long-range LoRa wireless telemetry
- LCD live monitoring display
- Automatic alert buzzer for unsafe conditions

---

## System Architecture
1. Sensor data acquired from wearable transmitter node
2. ESP32 processes and formats sensor packet
3. LoRa transmits packet wirelessly
4. Receiver decodes incoming packet
5. LCD displays readings
6. Buzzer activates during abnormal conditions

---

## Key Achievement
Reliable long-range underground telemetry with real-time hazard alert generation for worker safety monitoring.

---

## Future Scope
- Wearable enclosure integration
- Cloud dashboard logging
- GPS / underground localization support
- Multi-worker scalable network expansion

---

## Author
Pavan Pai
BMS College of Engineering
ECE Department
