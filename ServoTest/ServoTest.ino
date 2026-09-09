#include <Arduino.h>
#include <ESP32Servo.h>

// ==========================================
// SG90 Servo Simple Test
// ==========================================
// Wiring:
//   • Red Wire    --> ESP32 VIN (5V)
//   • Brown/Black --> ESP32 GND
//   • Orange      --> ESP32 GPIO 18
// ==========================================

const int SERVO_PIN = 18;
Servo myServo;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("======================================");
    Serial.println("     🤖 SG90 SERVO MOTION TEST        ");
    Serial.println("======================================");
    Serial.print("Attaching servo to GPIO ");
    Serial.println(SERVO_PIN);

    pinMode(2, OUTPUT); // Built-in Blue LED on GPIO 2
    digitalWrite(2, HIGH);

    // Standard 50Hz frequency and allocate all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);

    // Center position first
    Serial.println("Moving to Center (90°)...");
    myServo.write(90);
    delay(1500);

    Serial.println("Starting continuous sweep test!");
    Serial.println("======================================");
    Serial.flush();
}

void loop() {
    // 1. Move to 45 degrees (Looking Left)
    Serial.println("--> Looking Left (45°)");
    myServo.write(45);
    delay(1200);

    // 2. Move to 90 degrees (Looking Center)
    Serial.println("--> Looking Center (90°)");
    myServo.write(90);
    delay(1000);

    // 3. Move to 135 degrees (Looking Right)
    Serial.println("--> Looking Right (135°)");
    myServo.write(135);
    delay(1200);

    // 4. Return to Center
    Serial.println("--> Looking Center (90°)");
    myServo.write(90);
    delay(1000);

    // 5. Smooth sweep back and forth
    Serial.println("--> Smooth sweep from 45° to 135°...");
    for (int angle = 45; angle <= 135; angle += 2) {
        myServo.write(angle);
        delay(20);
    }
    delay(300);

    Serial.println("--> Smooth sweep from 135° to 45°...");
    for (int angle = 135; angle >= 45; angle -= 2) {
        myServo.write(angle);
        delay(20);
    }
    delay(500);

    Serial.println("--> Returning to Center (90°)");
    myServo.write(90);
    delay(2000);
}
