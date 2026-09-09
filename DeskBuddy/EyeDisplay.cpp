#include "EyeDisplay.h"

EyeDisplay::EyeDisplay()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
      isReady(false),
      currentMood(MOOD_IDLE),
      currentLookX(0.0f), currentLookY(0.0f),
      targetLookX(0.0f), targetLookY(0.0f),
      isBlinking(false), blinkProgress(0.0f), blinkClosing(true),
      nextBlinkTime(3000), nextGazeShiftTime(2500) {}

bool EyeDisplay::begin() {
    Serial.println();
    Serial.println("==========================================");
    Serial.println("[TEST] Checking breadboard electrical connection...");

    // Test for physical pull-up presence (proves power & breadboard contact)
    pinMode(OLED_SDA, INPUT_PULLDOWN);
    pinMode(OLED_SCL, INPUT_PULLDOWN);
    delay(10);
    bool sdaHasPower = (digitalRead(OLED_SDA) == HIGH);
    bool sclHasPower = (digitalRead(OLED_SCL) == HIGH);

    Serial.print("  • SDA (GPIO 21) Power/Contact: ");
    Serial.println(sdaHasPower ? "CONNECTED & POWERED (Detected OLED pull-up!)" : "❌ NO SIGNAL (OLED has NO power or wrong row!)");
    Serial.print("  • SCL (GPIO 22) Power/Contact: ");
    Serial.println(sclHasPower ? "CONNECTED & POWERED (Detected OLED pull-up!)" : "❌ NO SIGNAL (OLED has NO power or wrong row!)");
    Serial.println("==========================================");

    // Cleanly release pins from pulldown mode before starting I2C
    pinMode(OLED_SDA, INPUT);
    pinMode(OLED_SCL, INPUT);
    delay(5);

    // Initialize I2C at standard 100kHz
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(100000);
    Wire.setTimeOut(50);

    // Test specific OLED addresses first with detailed code
    Serial.println("[I2C] Probing 0x3C and 0x3D...");
    for (uint8_t testAddr : {0x3C, 0x3D}) {
        Wire.beginTransmission(testAddr);
        uint8_t err = Wire.endTransmission();
        Serial.print("  • Address 0x");
        Serial.print(testAddr, HEX);
        Serial.print(": ");
        if (err == 0) Serial.println("✅ ACK! (FOUND OLED DISPLAY!)");
        else if (err == 2) Serial.println("NACK (No device answered at this address)");
        else if (err == 5) Serial.println("⚠️ TIMEOUT (SDA/SCL lines may be swapped!)");
        else { Serial.print("Error code: "); Serial.println(err); }
    }

    uint8_t foundAddress = 0;
    int devicesCount = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("[I2C] >>> Device FOUND at address 0x");
            if (addr < 16) Serial.print("0");
            Serial.print(addr, HEX);
            Serial.println("! <<<");
            devicesCount++;
            if (foundAddress == 0) foundAddress = addr;
        }
    }

    if (devicesCount == 0) {
        Serial.println("[I2C] ❌ Zero I2C devices detected on the bus.");
        Serial.println("[I2C] ERROR: No I2C display responded at 0x3C or 0x3D!");
        Serial.println("[I2C] Checking pin electrical states:");
        Serial.print("      SDA (GPIO 21) reads: ");
        Serial.println(digitalRead(OLED_SDA) ? "HIGH (Normal)" : "LOW (⚠️ Line stuck or shorted to GND)");
        Serial.print("      SCL (GPIO 22) reads: ");
        Serial.println(digitalRead(OLED_SCL) ? "HIGH (Normal)" : "LOW (⚠️ Line stuck or shorted to GND)");
        Serial.println("[I2C] Common causes:");
        Serial.println("      1. SDA (Blue) and SCL (Yellow) are swapped.");
        Serial.println("      2. VCC wire is in 3.3V but needs 5V (try VIN).");
        Serial.println("      3. VCC and GND pins are reversed on your OLED board.");
        return false;
    }

    // Initialize SSD1306 display at the discovered address
    if (!display.begin(SSD1306_SWITCHCAPVCC, foundAddress)) {
        Serial.println("[OLED] SSD1306 allocation failed!");
        return false;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();

    nextBlinkTime = millis() + random(2000, 4000);
    nextGazeShiftTime = millis() + random(2000, 5000);

    isReady = true;
    return true;
}

void EyeDisplay::setMood(EyeMood mood) {
    currentMood = mood;
    if (mood == MOOD_HAPPY) {
        targetLookX = 0.0f;
        targetLookY = 0.0f;
    }
}

EyeMood EyeDisplay::getMood() const {
    return currentMood;
}

void EyeDisplay::setGaze(float x, float y) {
    targetLookX = constrain(x, -1.0f, 1.0f);
    targetLookY = constrain(y, -1.0f, 1.0f);
}

void EyeDisplay::triggerBlink() {
    isBlinking = true;
    blinkProgress = 0.0f;
    blinkClosing = true;
}

float EyeDisplay::getLookX() const {
    return currentLookX;
}

void EyeDisplay::update() {
    // Safety guard: do not access display buffer if OLED failed to initialize or buffer is NULL
    if (!isReady || display.getBuffer() == NULL) {
        return;
    }

    unsigned long now = millis();

    // 1. Smooth gaze interpolation
    currentLookX += (targetLookX - currentLookX) * 0.2f;
    currentLookY += (targetLookY - currentLookY) * 0.2f;

    // 2. Autonomous Idle behaviors (random glances & blinks)
    if (currentMood == MOOD_IDLE || currentMood == MOOD_SLEEPY) {
        // Gaze shift timer
        if (now >= nextGazeShiftTime) {
            int r = random(0, 5);
            if (r == 0) targetLookX = -0.8f;      // Glance left
            else if (r == 1) targetLookX = 0.8f; // Glance right
            else if (r == 2) targetLookX = 0.0f; // Look center
            else if (r == 3) targetLookY = -0.4f;// Look slightly up
            else targetLookY = 0.0f;

            nextGazeShiftTime = now + random(2500, 6000);
        }

        // Natural blink timer
        if (!isBlinking && now >= nextBlinkTime) {
            triggerBlink();
            nextBlinkTime = now + random(2500, 6000);
        }
    }

    // 3. Blinking animation state machine
    if (isBlinking) {
        const float blinkSpeed = 0.25f; // Fast, natural blink
        if (blinkClosing) {
            blinkProgress += blinkSpeed;
            if (blinkProgress >= 1.0f) {
                blinkProgress = 1.0f;
                blinkClosing = false;
            }
        } else {
            blinkProgress -= blinkSpeed;
            if (blinkProgress <= 0.0f) {
                blinkProgress = 0.0f;
                isBlinking = false;
            }
        }
    }

    // 4. Render frame
    display.clearDisplay();

    int leftEyeCenterX  = (SCREEN_WIDTH / 2) - (eyeWidth / 2) - (eyeSpacing / 2);
    int rightEyeCenterX = (SCREEN_WIDTH / 2) + (eyeWidth / 2) + (eyeSpacing / 2);
    int eyeCenterY      = SCREEN_HEIGHT / 2;

    float openRatio = 1.0f - blinkProgress;

    switch (currentMood) {
        case MOOD_HAPPY:
            drawHappyEye(leftEyeCenterX, eyeCenterY);
            drawHappyEye(rightEyeCenterX, eyeCenterY);
            break;

        case MOOD_SURPRISED:
            drawSurprisedEye(leftEyeCenterX, eyeCenterY, currentLookX, currentLookY);
            drawSurprisedEye(rightEyeCenterX, eyeCenterY, currentLookX, currentLookY);
            break;

        case MOOD_SLEEPY:
            drawSleepyEye(leftEyeCenterX, eyeCenterY);
            drawSleepyEye(rightEyeCenterX, eyeCenterY);
            break;

        case MOOD_SQUINT:
            drawEye(leftEyeCenterX, eyeCenterY, currentLookX, currentLookY, 0.35f * openRatio);
            drawEye(rightEyeCenterX, eyeCenterY, currentLookX, currentLookY, 0.35f * openRatio);
            break;

        case MOOD_IDLE:
        default:
            drawEye(leftEyeCenterX, eyeCenterY, currentLookX, currentLookY, openRatio);
            drawEye(rightEyeCenterX, eyeCenterY, currentLookX, currentLookY, openRatio);
            break;
    }

    display.display();
}

void EyeDisplay::drawEye(int centerX, int centerY, float lookOffsetX, float lookOffsetY, float openRatio) {
    int currentH = (int)(eyeHeight * openRatio);
    if (currentH < 3) {
        // Completely closed eye (thin slit line)
        display.drawFastHLine(centerX - eyeWidth / 2, centerY, eyeWidth, SSD1306_WHITE);
        display.drawFastHLine(centerX - eyeWidth / 2, centerY + 1, eyeWidth, SSD1306_WHITE);
        return;
    }

    int topY = centerY - currentH / 2;
    int r = eyeRadius;
    if (r > currentH / 2) r = currentH / 2;

    // 1. Draw outer white eye capsule
    display.fillRoundRect(centerX - eyeWidth / 2, topY, eyeWidth, currentH, r, SSD1306_WHITE);

    // 2. Pupil calculation (moves with gaze)
    if (openRatio > 0.4f) {
        int maxOffsetX = (eyeWidth / 2) - 8;
        int maxOffsetY = (currentH / 2) - 8;
        int pupilX = centerX + (int)(lookOffsetX * maxOffsetX);
        int pupilY = centerY + (int)(lookOffsetY * maxOffsetY);
        int pupilR = 7;

        // Dark pupil
        display.fillCircle(pupilX, pupilY, pupilR, SSD1306_BLACK);

        // Specular highlight (cute gleam reflection in pupil)
        display.fillCircle(pupilX + 2, pupilY - 2, 2, SSD1306_WHITE);
    }
}

void EyeDisplay::drawHappyEye(int centerX, int centerY) {
    // Cheerful inverted crescent arc (^ shape)
    int w = eyeWidth;
    for (int i = 0; i < 4; i++) {
        // Draw nested arcs for a clean, thick friendly stroke
        display.drawCircle(centerX, centerY + 8 - i, w / 2, SSD1306_WHITE);
    }
    // Mask out the bottom half so only the upper curved arch remains
    display.fillRect(centerX - w / 2 - 2, centerY + 9, w + 4, 25, SSD1306_BLACK);

    // Little happy cheek blush dot
    display.fillCircle(centerX, centerY + 22, 2, SSD1306_WHITE);
}

void EyeDisplay::drawSurprisedEye(int centerX, int centerY, float lookOffsetX, float lookOffsetY) {
    // Wide open circular eyes (O_O)
    int r = 24;
    display.fillCircle(centerX, centerY, r, SSD1306_WHITE);

    int pupilX = centerX + (int)(lookOffsetX * 6);
    int pupilY = centerY + (int)(lookOffsetY * 6);
    display.fillCircle(pupilX, pupilY, 10, SSD1306_BLACK);
    display.fillCircle(pupilX + 3, pupilY - 3, 3, SSD1306_WHITE);
}

void EyeDisplay::drawSleepyEye(int centerX, int centerY) {
    // Drooping eyelids covering top 60%
    int topY = centerY - eyeHeight / 2;
    display.fillRoundRect(centerX - eyeWidth / 2, topY, eyeWidth, eyeHeight, eyeRadius, SSD1306_WHITE);

    // Black rectangle masking off upper eyelid
    int lidHeight = (int)(eyeHeight * 0.65f);
    display.fillRect(centerX - eyeWidth / 2 - 1, topY - 1, eyeWidth + 2, lidHeight, SSD1306_BLACK);

    // Sleepy lower pupil
    display.fillCircle(centerX, centerY + 8, 5, SSD1306_BLACK);
}
