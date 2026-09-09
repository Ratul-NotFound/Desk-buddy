#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. OLED Display (I2C SSD1306 0.96")
// ==========================================
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C
#define OLED_SDA        21      // ESP32 I2C Data
#define OLED_SCL        22      // ESP32 I2C Clock

// ==========================================
// 2. SG90 Servo (PWM)
// ==========================================
#define SERVO_PIN        18     // PWM pin for servo
#define SERVO_MIN_ANGLE  40.0f  // Physical limit (degrees)
#define SERVO_MAX_ANGLE  140.0f // Physical limit (degrees)
#define SERVO_CENTER     90.0f  // Center forward position
#define SERVO_EASING     0.15f  // Smooth cubic easing factor (protects USB power)

// ==========================================
// 3. Audio Amplifier (PAM8403 + 3W Speaker)
// ==========================================
#define AUDIO_DAC_PIN    25     // ESP32 Built-in Hardware DAC 1

// ==========================================
// 4. Interactive Sensors
// ==========================================
#define ENABLE_SOUND_SENSOR  true    // Microphone module connected to GPIO 19
#define MIC_DO_PIN           19      // Digital trigger pin (LM393 / KY-038 Sound Sensor)
#define ENABLE_TOUCH_PIN     true    // ESP32 Hardware Capacitive Touch (GPIO 4 - Touch 0)
#define TOUCH_HEAD_PIN       4       // Connect jumper wire to GPIO 4 to pet robot's head!

// ==========================================
// 5. Dual-Mode WiFi & Cloud AI Settings
// ==========================================
#define AP_DEFAULT_SSID      "Piku-WiFi"
#define AP_DEFAULT_PASS      "piku12345"
#define MDNS_HOSTNAME        "piku"
#define NVS_NAMESPACE        "piku"

#define NTP_SERVER           "pool.ntp.org"
#define DEFAULT_GMT_OFFSET   0       // Seconds (e.g. 0 for UTC, or customize in dashboard)
#define DEFAULT_DAYLIGHT_OFFSET 0

#endif // CONFIG_H
