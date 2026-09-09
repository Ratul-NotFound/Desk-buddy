#ifndef SERVO_MOTION_H
#define SERVO_MOTION_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

class ServoMotion {
public:
    ServoMotion();

    // Attach servo and set initial center position
    void begin(int pin = SERVO_PIN);

    // Call continuously in loop() to compute smooth interpolated steps
    void update();

    // Directly set target angle (degrees: 0 - 180, clamped to safe limits)
    void setTargetAngle(float angle);

    // Coordinate physical head direction with OLED eye gaze (-1.0 to 1.0)
    void lookWithEyes(float lookX);

    // Trigger playful happy celebration wiggle dance
    void triggerWiggle();

    // Get current interpolated angle
    float getCurrentAngle() const;

private:
    Servo servo;
    float currentAngle;
    float targetAngle;
    unsigned long lastUpdateTime;

    // Wiggle dance state
    bool isWiggling;
    int wiggleStep;
    unsigned long nextWiggleStepTime;
};

#endif // SERVO_MOTION_H
