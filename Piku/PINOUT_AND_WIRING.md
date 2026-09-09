# 🔌 PIKU: Pinout & Complete Hardware Wiring Guide

This document contains the complete, verified wiring schematic for **PIKU** (Living EMO-Style AI Desktop Companion) powered by a **30-Pin ESP32 DevKit V1**, **0.96" I2C OLED (SSD1306)**, **SG90 9g Servo Motor**, **PAM8403 3W Audio Amplifier**, and **Microphone Sound Sensor**.

---

## 📋 Master Pin Connection Table

| Module / Component | Module Pin | ESP32 DevKit Pin | Pin Function | Power / Voltage | Wire Color (Recommended) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **0.96" I2C OLED** | `GND` | `GND` | Ground | 0V | Black |
| | `VCC` | `3V3` (or `VIN`) | Power | 3.3V – 5.0V | Red |
| | `SCL` | **`GPIO 22`** | I2C Clock | 3.3V Logic | Yellow |
| | `SDA` | **`GPIO 21`** | I2C Data | 3.3V Logic | Blue |
| **SG90 9g Servo** | `GND` (Brown/Black) | `GND` | Ground | 0V | Black / Brown |
| | `VCC` (Red) | **`VIN` / `5V`** | Main 5V Rail | 5V DC | Red |
| | `PWM` (Orange/Yellow) | **`GPIO 18`** | Hardware PWM | 3.3V PWM Signal | Orange / Yellow |
| **PAM8403 Amp** | `+5V` / `VCC` | **`VIN` / `5V`** | Main 5V Rail | 5V DC | Red |
| | `GND` | `GND` | Ground | 0V | Black |
| | `L_IN` or `R_IN` | **`GPIO 25`** | Built-in DAC 1 | Analog 8-bit Audio | Purple / Green |
| | `Speaker + / -` | To 3W 4Ω/8Ω Speaker | Audio Output | Differential AC | White / Grey |
| **Microphone Sensor** | `GND` | `GND` | Ground | 0V | Black |
| | `VCC` | `3V3` (or `5V`) | Power | 3.3V – 5.0V | Red |
| | `DO` (Digital Out) | **`GPIO 19`** | Clap / Sound Trigger | 3.3V Logic (Active LOW) | Green |
| **Capacitive Touch** | Top Head Wire | **`GPIO 4`** | Touch 0 Sensor | Capacitive Petting | Copper / Bare Wire |

---

## ⚡ Power Supply & Stability Guidelines

1. **Single USB Cable 5V Rail:**
   - The entire robot (ESP32, OLED, SG90 Servo, PAM8403 Amp, and Mic) is engineered to run from a **single 5V USB cable or power bank** (~500mA–1A).
2. **Servo Power (`VIN` vs `3V3`):**
   - **NEVER** connect the Servo `VCC` (Red wire) to `3V3`! The servo draws peak currents of ~250mA, which will overheat and damage the ESP32's onboard 3.3V voltage regulator. Connect Servo `VCC` directly to **`VIN` (5V)**.
3. **Common Ground:**
   - Ensure all `GND` lines (ESP32, OLED, Servo, Amplifier, Microphone) are connected together to maintain a clean ground reference.
4. **Microphone Sensitivity Tuning:**
   - The microphone module has a small blue potentiometer screw. Turn it gently with a small screwdriver until the onboard sensor LED turns OFF during quiet room conditions and flashes ON when you clap your hands.

---

## 📐 Circuit Diagram Visual

```
                     ┌────────────────────────────────────────┐
                     │          ESP32 DevKit (30-Pin)         │
                     │                                        │
  [OLED SDA] ────────┤ GPIO 21                         GPIO 22├──────── [OLED SCL]
  [Audio DAC] ───────┤ GPIO 25                         GPIO 19├──────── [Mic Sensor DO]
  [Head Touch] ──────┤ GPIO 4                          GPIO 18├──────── [Servo PWM]
  [Common GND] ──────┤ GND                                 VIN├──────── [5V Rail: Servo & Amp]
  [OLED & Mic 3V3] ──┤ 3V3                                    │
                     └────────────────────────────────────────┘
```
