# 🤖 PIKU: Ultimate Living AI Desktop Companion

**PIKU** is an autonomous, expressive robotic desk pet inspired by the **LivingAI EMO** companion. Powered by an ESP32, it combines Google Gemini 1.5 Flash AI, 16kHz studio voice output, dual-mode WiFi, interactive OLED vector expressions, capacitive touch petting, sound/clap recognition, and smooth servo head body language.

---

## 🌟 Top Features

- **🧠 Google Gemini 1.5 Flash AI Brain:** Direct HTTPS cloud integration with voice speech-to-text input, emotion parsing, and real-time scrolling answers.
- **📡 Dual-Mode Portable WiFi (`WIFI_AP_STA`):** Direct AP hotspot (`Piku-WiFi` at `192.168.4.1`) + router provisioning with flash memory auto-reconnect (`http://piku.local`).
- **🎤 Sound & Clap Engine:** Single clap wake-up, double-clap DJ party mode, and intelligent self-muting during servo/speaker motion.
- **✋ Capacitive Touch Petting:** Touch GPIO 4 to pet PIKU's head for purring cat expressions and wiggle dances.
- **🗣️ 21 16kHz HD Studio Voice Tracks:** High-pitched, playful robotic vocalizations via hardware DAC (GPIO 25) with zero jitter or distortion.
- **👀 18 Vector OLED Expressions:** Biological breathing, eye saccades, double-blinking, and Pixar-style emotional deforming.
- **🕹️ Playable Mini-Games:** On-screen Flappy Bird, Rock-Paper-Scissors, and Magic 8-Ball.
- **☀️ Live NTP Clock & Weather:** Synchronized real-time clock and Open-Meteo live temperature with weather-reactive moods.

---

## 📁 Folder Contents

- **`Piku.ino`** — Master autonomous companion sketch with Dual-Mode WiFi, Web App, AI Engine, Games, and Expression state machine.
- **`config.h`** — Hardware pin mappings and safety configuration limits.
- **`voice_samples.h`** — 21 16kHz studio HD audio arrays in PROGMEM.
- **`PINOUT_AND_WIRING.md`** — Complete pin connection table, power guidelines, and circuit diagrams.
- **`SETUP_AND_USER_GUIDE.md`** — Step-by-step flashing guide, WiFi router provisioning, Gemini AI setup, and command reference.

---

## 🚀 Quickstart

1. Open **`Piku.ino`** in Arduino IDE.
2. Select **Tools ➔ Partition Scheme ➔ "Huge APP (3MB No OTA/1MB SPIFFS)"**.
3. Upload to your ESP32.
4. Connect your phone to **`Piku-WiFi`** (pass: `piku12345`) and open **`http://192.168.4.1`**!
