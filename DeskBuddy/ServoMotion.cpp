#include "ServoMotion.h"

ServoMotion::ServoMotion()
    : currentAngle(SERVO_CENTER),
      targetAngle(SERVO_CENTER),
      lastUpdateTime(0),
      isWiggling(false),
      wiggleStep(0),
      nextWiggleStepTime(0) {}

void ServoMotion::begin(int pin) {
    // Allocate hardware PWM timer for ESP32
    ESP32PWM::allocateTimer(0);
    servo.setPeriodHertz(50); // Standard 50Hz servo frequency

    // Attach SG90 servo (500us to 2400us standard pulse width)
    servo.attach(pin, 500, 2400);

    currentAngle = SERVO_CENTER;
    targetAngle = SERVO_CENTER;
    servo.write((int)SERVO_CENTER);

    lastUpdateTime = millis();
}

void ServoMotion::setTargetAngle(float angle) {
    targetAngle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
}

void ServoMotion::lookWithEyes(float lookX) {
    if (isWiggling) return; // Don't interrupt dance with gaze shifts

    // Map -1.0..1.0 gaze to physical head rotation
    // -1.0 (Left) -> 90° - 35° = 55°
    // +1.0 (Right) -> 90° + 35° = 125°
    float angle = SERVO_CENTER + (lookX * 35.0f);
    setTargetAngle(angle);
}

void ServoMotion::triggerWiggle() {
    isWiggling = true;
    wiggleStep = 0;
    nextWiggleStepTime = millis();
}

float ServoMotion::getCurrentAngle() const {
    return currentAngle;
}

void ServoMotion::update() {
    unsigned long now = millis();

    // Step at smooth 20ms intervals (50 updates/sec)
    if (now - lastUpdateTime < 20) {
        return;
    }
    lastUpdateTime = now;

    // Handle happy wiggle dance routine
    if (isWiggling && now >= nextWiggleStepTime) {
        switch (wiggleStep) {
            case 0: setTargetAngle(SERVO_CENTER - 18.0f); nextWiggleStepTime = now + 140; break;
            case 1: setTargetAngle(SERVO_CENTER + 18.0f); nextWiggleStepTime = now + 140; break;
            case 2: setTargetAngle(SERVO_CENTER - 12.0f); nextWiggleStepTime = now + 120; break;
            case 3: setTargetAngle(SERVO_CENTER + 12.0f); nextWiggleStepTime = now + 120; break;
            case 4: setTargetAngle(SERVO_CENTER);         nextWiggleStepTime = now + 100; break;
            default:
                isWiggling = false;
                wiggleStep = 0;
                break;
        }
        if (isWiggling) wiggleStep++;
    }

    // Smooth cubic/sinusoidal easing toward target angle
    float diff = targetAngle - currentAngle;
    if (fabs(diff) > 0.3f) {
        currentAngle += diff * SERVO_EASING;
        servo.write((int)round(currentAngle));
    }
}
