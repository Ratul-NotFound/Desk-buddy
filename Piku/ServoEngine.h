#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

enum GestureType {
    GESTURE_NOD,
    GESTURE_SHAKE,
    GESTURE_TILT_LEFT,
    GESTURE_TILT_RIGHT,
    GESTURE_CURIOUS,
    GESTURE_WIGGLE
};

class ServoEngine {
public:
    void init();
    void setTarget(float angle);
    float getCurrent() const { return _current; }
    void triggerWiggle();
    void performGesture(GestureType g);
    void update();          // call every ~20ms from soul loop

private:
    Servo _servo;
    float _current   = SERVO_CENTER;
    float _target    = SERVO_CENTER;
    bool  _wiggling  = false;
    unsigned long _wiggleEnd  = 0;
    int   _wigglePhase        = 0;
    unsigned long _nextWiggle = 0;
};
