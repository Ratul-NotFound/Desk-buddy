#include <Servo.h>

// =======================================================
// SG90 Servo - Fast Full Range Sweep & Speed Test
// =======================================================
// Sweeps rapidly across the full range (10° to 170°)
// to quickly break in and free up internal gears.
//
// Wiring on Arduino Uno:
//   • Red Wire    --> Arduino Uno 5V
//   • Brown/Black --> Arduino Uno GND
//   • Orange      --> Arduino Uno Pin 9
// =======================================================

const int SERVO_PIN = 9;
Servo myServo;

void setup() {
    Serial.begin(9600);
    delay(500);

    Serial.println("==========================================");
    Serial.println("   ⚡ SG90 FAST FULL-RANGE ROTATIONS      ");
    Serial.println("==========================================");

    // Standard SG90 pulse range
    myServo.attach(SERVO_PIN, 544, 2400);

    myServo.write(90);
    delay(1000);
}

void loop() {
    // 1. Ultra-Fast Direct Snaps (Full Motor Power)
    Serial.println(">> Rapid Snaps: 15° <--> 165°");
    for (int i = 0; i < 5; i++) {
        myServo.write(15);
        delay(350);
        myServo.write(165);
        delay(350);
    }

    // 2. High-Speed Smooth Sweep (4ms per degree)
    Serial.println(">> High-Speed Smooth Sweeps...");
    for (int pass = 0; pass < 5; pass++) {
        // Fast sweep forward (10° -> 170°)
        for (int a = 10; a <= 170; a += 2) {
            myServo.write(a);
            delay(4); // 4ms per 2 degrees = ~0.3 seconds full sweep!
        }
        delay(100);

        // Fast sweep backward (170° -> 10°)
        for (int a = 170; a >= 10; a -= 2) {
            myServo.write(a);
            delay(4);
        }
        delay(100);
    }

    // 3. Center reset
    myServo.write(90);
    delay(500);
}
