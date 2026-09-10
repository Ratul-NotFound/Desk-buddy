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
    if (!_gestureActive) {
        _target = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    }
}

void ServoEngine::triggerWiggle() {
    performGesture(GESTURE_WIGGLE);
}

void ServoEngine::performGesture(GestureType g) {
    _currentGesture = g;
    _gestureActive  = true;
    _gestureStep    = 0;
    _nextStepTime   = millis();
}

void ServoEngine::update() {
    unsigned long now = millis();

    if (_gestureActive && now >= _nextStepTime) {
        switch (_currentGesture) {
            case GESTURE_NOD: {
                // Multi-frame nod: down, up, down, center
                float angles[] = { 72.0f, 104.0f, 75.0f, 90.0f };
                int delays[]   = { 220,   220,    200,   200 };
                if (_gestureStep < 4) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_SHAKE: {
                // Multi-frame head shake: left, right, left, right, center
                float angles[] = { 112.0f, 68.0f, 108.0f, 72.0f, 90.0f };
                int delays[]   = { 180,    180,   160,    160,   200 };
                if (_gestureStep < 5) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_TILT_LEFT: {
                float angles[] = { 68.0f, 90.0f };
                int delays[]   = { 1100,  300 };
                if (_gestureStep < 2) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_TILT_RIGHT: {
                float angles[] = { 112.0f, 90.0f };
                int delays[]   = { 1100,   300 };
                if (_gestureStep < 2) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_CURIOUS: {
                // Curious tilt with hold
                float angles[] = { 116.0f, 110.0f, 90.0f };
                int delays[]   = { 300,    1300,   350 };
                if (_gestureStep < 3) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_WIGGLE: {
                // High-energy dance wiggle
                float angles[] = { 115.0f, 65.0f, 112.0f, 68.0f, 105.0f, 75.0f, 90.0f };
                int delays[]   = { 130,    130,   120,    120,   110,    110,   200 };
                if (_gestureStep < 7) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_PURR: {
                // Gentle swaying for petting
                float angles[] = { 84.0f, 96.0f, 86.0f, 94.0f, 90.0f };
                int delays[]   = { 350,   350,   300,   300,   250 };
                if (_gestureStep < 5) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_YAWN: {
                // Slow droop, stretch, and recover
                float angles[] = { 74.0f, 72.0f, 94.0f, 90.0f };
                int delays[]   = { 600,   1400,  400,   300 };
                if (_gestureStep < 4) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_STARTLE: {
                // Quick jerk back and recover
                float angles[] = { 114.0f, 80.0f, 98.0f, 90.0f };
                int delays[]   = { 120,    140,   160,   200 };
                if (_gestureStep < 4) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
            case GESTURE_CONFUSED: {
                // Inquisitive tilt left, pause, tilt right, center
                float angles[] = { 76.0f, 76.0f, 104.0f, 90.0f };
                int delays[]   = { 300,   800,   900,    300 };
                if (_gestureStep < 4) {
                    _target = angles[_gestureStep];
                    _nextStepTime = now + delays[_gestureStep];
                    _gestureStep++;
                } else {
                    _gestureActive = false;
                    _target = SERVO_CENTER;
                }
                break;
            }
        }
    }

    _current += (_target - _current) * SERVO_EASING;
    int angle = constrain((int)_current, (int)SERVO_MIN_ANGLE, (int)SERVO_MAX_ANGLE);
    _servo.write(angle);
}
