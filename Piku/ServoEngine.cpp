#include "ServoEngine.h"

void ServoEngine::init() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    _servo.setPeriodHertz(50);
    _servo.attach(SERVO_PIN, 500, 2400);
    _servo.write((int)SERVO_CENTER);
    _current = SERVO_CENTER;
    _target  = SERVO_CENTER;
}

void ServoEngine::setTarget(float angle) {
    _target = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
}

void ServoEngine::triggerWiggle() {
    _wiggling   = true;
    _wiggleEnd  = millis() + 800;
    _wigglePhase = 0;
    _nextWiggle = millis();
}

void ServoEngine::performGesture(GestureType g) {
    switch (g) {
        case GESTURE_NOD:
            setTarget(SERVO_CENTER - 15.0f);
            break;
        case GESTURE_SHAKE:
            triggerWiggle();
            break;
        case GESTURE_TILT_LEFT:
            setTarget(SERVO_CENTER - 20.0f);
            break;
        case GESTURE_TILT_RIGHT:
            setTarget(SERVO_CENTER + 20.0f);
            break;
        case GESTURE_CURIOUS:
            setTarget(SERVO_CENTER + 25.0f);
            break;
        case GESTURE_WIGGLE:
            triggerWiggle();
            break;
    }
}

void ServoEngine::update() {
    unsigned long now = millis();
    if (_wiggling) {
        if (now >= _wiggleEnd) {
            _wiggling = false;
            _target = SERVO_CENTER;
        } else if (now >= _nextWiggle) {
            _nextWiggle = now + 120;
            _wigglePhase = (_wigglePhase + 1) % 4;
            float offsets[] = { 20.0f, -20.0f, 15.0f, -15.0f };
            _target = SERVO_CENTER + offsets[_wigglePhase];
        }
    }
    _current += (_target - _current) * SERVO_EASING;
    int angle = constrain((int)_current, (int)SERVO_MIN_ANGLE, (int)SERVO_MAX_ANGLE);
    _servo.write(angle);
}
