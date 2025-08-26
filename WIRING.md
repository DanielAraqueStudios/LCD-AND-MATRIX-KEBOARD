# 🍔 ESP32-S3 Hamburger Grill System - Complete Wiring Guide

## Professional Mechatronics Wiring for Automated Grill Control

### Overview
This document provides comprehensive wiring instructions for the ESP32-S3 Hamburger Grill Control System, including matrix keyboard, LCD display, and temperature sensor emulation components.

## 🔌 Complete Hardware Connections

### ESP32-S3 GPIO Pin Assignments

| Component | Function | GPIO Pin | External Pin | Description |
|-----------|----------|----------|--------------|-------------|
| **Matrix Keyboard** | Row 0 | GPIO_1 | Pin 1 | Output for row scanning |
| **Matrix Keyboard** | Row 1 | GPIO_2 | Pin 5 | Output for row scanning |
| **Matrix Keyboard** | Row 2 | GPIO_42 | Pin 2 | Output for row scanning |
| **Matrix Keyboard** | Row 3 | GPIO_41 | Pin 6 | Output for row scanning |
| **Matrix Keyboard** | Col 0 | GPIO_40 | Pin 3 | Input with internal pullup |
| **Matrix Keyboard** | Col 1 | GPIO_39 | Pin 7 | Input with internal pullup |
| **Matrix Keyboard** | Col 2 | GPIO_38 | Pin 4 | Input with internal pullup |
| **Matrix Keyboard** | Col 3 | GPIO_37 | Pin 8 | Input with internal pullup |
| **LCD Display** | RS | GPIO_10 | Pin 4 | Register Select |
| **LCD Display** | EN | GPIO_11 | Pin 6 | Enable Signal |
| **LCD Display** | D4 | GPIO_16 | Pin 11 | Data bit 4 |
| **LCD Display** | D5 | GPIO_17 | Pin 12 | Data bit 5 |
| **LCD Display** | D6 | GPIO_18 | Pin 13 | Data bit 6 |
| **LCD Display** | D7 | GPIO_7 | Pin 14 | Data bit 7 |
| **Temperature Sensor** | ADC Input | GPIO_36 | Wiper | Potentiometer for temperature emulation |
