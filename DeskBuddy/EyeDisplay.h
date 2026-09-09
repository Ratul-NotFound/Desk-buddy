#ifndef EYE_DISPLAY_H
#define EYE_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

enum EyeMood {
    MOOD_IDLE,
    MOOD_HAPPY,
    MOOD_SURPRISED,
    MOOD_SLEEPY,
    MOOD_SQUINT
};

class EyeDisplay {
public:
    EyeDisplay();

    // Initialize OLED display via I2C
    bool begin();

    // Check if display initialized successfully
    bool isAvailable() const { return isReady; }

    // Update animations, blinks, and screen refresh (call in loop)
    void update();

    // Set emotional expression
    void setMood(EyeMood mood);
    EyeMood getMood() const;

    // Direct eye gaze control (-1.0 = left, 0.0 = center, +1.0 = right)
    void setGaze(float x, float y);

    // Trigger an immediate manual blink
    void triggerBlink();

    // Get current horizontal gaze (-1.0 to 1.0) for servo head tracking
    float getLookX() const;

private:
    Adafruit_SSD1306 display;
    bool isReady;

    EyeMood currentMood;

    // Eye geometry dimensions
    const int eyeWidth  = 34;
    const int eyeHeight = 42;
    const int eyeRadius = 10;
    const int eyeSpacing = 16; // Distance between inner edges

    // Gaze tracking
    float currentLookX;
    float currentLookY;
    float targetLookX;
    float targetLookY;

    // Blinking state machine
    bool isBlinking;
    float blinkProgress; // 0.0 = fully open, 1.0 = fully closed
    bool blinkClosing;
    unsigned long nextBlinkTime;
    unsigned long nextGazeShiftTime;

    // Internal draw methods
    void drawEye(int centerX, int centerY, float lookOffsetX, float lookOffsetY, float openRatio);
    void drawHappyEye(int centerX, int centerY);
    void drawSurprisedEye(int centerX, int centerY, float lookOffsetX, float lookOffsetY);
    void drawSleepyEye(int centerX, int centerY);
};

#endif // EYE_DISPLAY_H
