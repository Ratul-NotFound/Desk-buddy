#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ========================================================
// 🎭 TRUE FRAME-SYNCHRONIZED EMOTION & AUDIO ENGINE (Uno)
// ========================================================
// Every single video frame is locked 1:1 with real-time audio:
//
// 1. 😄 HAPPY: Sound pitch pitches UP & DOWN in lockstep
//              with the bouncing eyes on every frame!
//
// 2. 😭 SAD:   Whining tone glides down with the tear,
//              and plays a "plink!" drop sound the exact
//              moment the teardrop hits the bottom!
//
// 3. 😡 ANGRY: Low grinder buzz vibrates on the exact
//              frames the screen shakes, with a sharp zap
//              when the anger vein (💢) pulses!
//
// 4. 😍 LOVE:  Heartbeat audio ("LUB-DUB") plays at the
//              EXACT moment the heart expands and beats!
//
// 5. 😱 SHOCK: Pitch sweeps upward from 800 to 3000 Hz
//              in direct sync with the dilating pupils!
//
// 6. 😴 SLEEP: Gentle rhythmic breathing hum rises and
//              falls in lockstep with the floating Zzz!
//
// Commands in Serial Monitor (9600 baud):
//   'h' -> HAPPY 😄
//   's' -> SAD 😭
//   'a' -> ANGRY 😡
//   'o' -> SHOCKED 😱
//   'l' -> IN LOVE 😍
//   'z' -> SLEEPY 😴
//   '0' -> Auto-Cycle
// ========================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int AUDIO_PIN = 11; // Tone pin to PAM8403 L_IN
const int LX = 38;        // Left eye center X
const int RX = 90;        // Right eye center X
const int EY = 26;        // Eye center Y

int currentMode = 0; // 0 = Auto-cycle
unsigned long modeTimer = 0;
int autoMood = 1;

// --------------------------------------------------------
// GRAPHICS HELPERS
// --------------------------------------------------------
void drawStar(int x, int y, int r) {
    display.drawLine(x - r, y, x + r, y, SSD1306_WHITE);
    display.drawLine(x, y - r, x, y + r, SSD1306_WHITE);
    if (r > 2) {
        display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
        display.drawPixel(x + 1, y - 1, SSD1306_WHITE);
        display.drawPixel(x - 1, y + 1, SSD1306_WHITE);
        display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
    }
}

void drawAngerVein(int x, int y, int sz) {
    display.drawFastHLine(x - sz, y - sz/2, sz, SSD1306_WHITE);
    display.drawFastVLine(x - sz/2, y - sz, sz, SSD1306_WHITE);
    display.drawFastHLine(x, y - sz/2, sz, SSD1306_WHITE);
    display.drawFastVLine(x + sz/2, y - sz, sz, SSD1306_WHITE);
    display.drawFastHLine(x - sz, y + sz/2, sz, SSD1306_WHITE);
    display.drawFastVLine(x - sz/2, y, sz, SSD1306_WHITE);
    display.drawFastHLine(x, y + sz/2, sz, SSD1306_WHITE);
    display.drawFastVLine(x + sz/2, y, sz, SSD1306_WHITE);
}

void drawHeart(int cx, int cy, int size) {
    int r = size / 2;
    display.fillCircle(cx - r/2, cy - r/2, r/2, SSD1306_WHITE);
    display.fillCircle(cx + r/2, cy - r/2, r/2, SSD1306_WHITE);
    display.fillTriangle(cx - size/2 - 1, cy - r/4, cx + size/2 + 1, cy - r/4, cx, cy + size/2, SSD1306_WHITE);
}

// --------------------------------------------------------
// 1. 😄 HAPPY (Pitch Locked to Bouncing Eyes)
// --------------------------------------------------------
void renderHappyFrame(int f) {
    display.clearDisplay();
    int bounceY = (f % 4 < 2) ? -2 : 1;

    // Pitch changes in exact lockstep with eye position!
    if (bounceY == -2) {
        tone(AUDIO_PIN, 1760 + (f * 80)); // High cheerful pitch when bouncing up!
    } else {
        tone(AUDIO_PIN, 1320);           // Lower pitch when landing
    }
    if (f == 7) noTone(AUDIO_PIN);

    // Smiling eyes
    for (int i = 0; i < 4; i++) {
        display.drawCircle(LX, EY + 8 - i + bounceY, 17, SSD1306_WHITE);
        display.drawCircle(RX, EY + 8 - i + bounceY, 17, SSD1306_WHITE);
    }
    display.fillRect(LX - 19, EY + 9 + bounceY, 38, 25, SSD1306_BLACK);
    display.fillRect(RX - 19, EY + 9 + bounceY, 38, 25, SSD1306_BLACK);

    display.fillCircle(LX - 10, EY + 20 + bounceY, 3, SSD1306_WHITE);
    display.fillCircle(RX + 10, EY + 20 + bounceY, 3, SSD1306_WHITE);

    display.fillRoundRect(54, 48 + bounceY, 20, 12, 6, SSD1306_WHITE);
    display.fillCircle(64, 50 + bounceY, 4, SSD1306_BLACK);

    int starSize = (f % 2 == 0) ? 4 : 2;
    drawStar(14, 14, starSize);
    drawStar(114, 12, starSize);
}

// --------------------------------------------------------
// 2. 😭 SAD (Whining Tone Down + "Plink" Drop Sound at Bottom)
// --------------------------------------------------------
void renderSadFrame(int t) {
    display.clearDisplay();

    // Eye capsules
    display.fillRoundRect(LX - 16, EY - 12, 32, 34, 10, SSD1306_WHITE);
    display.fillRoundRect(RX - 16, EY - 12, 32, 34, 10, SSD1306_WHITE);

    // Sad drooping eyelid cuts (/ \)
    display.fillTriangle(LX - 18, EY - 14, LX + 18, EY - 14, LX - 18, EY - 2, SSD1306_BLACK);
    display.fillTriangle(RX - 18, EY - 14, RX + 18, EY - 14, RX + 18, EY - 2, SSD1306_BLACK);

    display.fillCircle(LX, EY + 5, 6, SSD1306_BLACK);
    display.fillCircle(RX, EY + 5, 6, SSD1306_BLACK);
    display.fillCircle(LX + 2, EY + 3, 2, SSD1306_WHITE);
    display.fillCircle(RX + 2, EY + 3, 2, SSD1306_WHITE);

    int quiver = (t % 2 == 0) ? 1 : 0;
    for (int i = 0; i < 3; i++) {
        display.drawCircle(64, 60 + i + quiver, 10, SSD1306_WHITE);
    }
    display.fillRect(50, 58 + quiver, 28, 14, SSD1306_BLACK);

    // Teardrop position: drops from 34 down to 60
    int tY = 34 + (t * 2);
    display.fillCircle(LX + 12, tY, 3, SSD1306_WHITE);
    display.fillTriangle(LX + 12 - 2, tY, LX + 12 + 2, tY, LX + 12, tY - 4, SSD1306_WHITE);
    display.fillCircle(RX - 12, tY, 3, SSD1306_WHITE);
    display.fillTriangle(RX - 12 - 2, tY, RX - 12 + 2, tY, RX - 12, tY - 4, SSD1306_WHITE);

    // Audio in lockstep with teardrop:
    if (t < 10) {
        // Whining pitch drops as tear slides down
        tone(AUDIO_PIN, 900 - (t * 45));
    } else {
        // Teardrop hits the floor: water "plink" sound!
        tone(AUDIO_PIN, 1750);
    }
}

// --------------------------------------------------------
// 3. 😡 ANGRY (Grinding Rumble on Shake + Zap on Vein Pulse)
// --------------------------------------------------------
void renderAngryFrame(int r) {
    display.clearDisplay();

    int ox = (r % 2 == 0) ? -2 : 2;
    int oy = (r % 3 == 0) ? 1 : -1;

    int curLX = LX + ox;
    int curRX = RX + ox;
    int curEY = EY + oy;

    display.fillRoundRect(curLX - 17, curEY - 18, 34, 38, 8, SSD1306_WHITE);
    display.fillRoundRect(curRX - 17, curEY - 18, 34, 38, 8, SSD1306_WHITE);

    display.fillTriangle(curLX - 19, curEY - 20, curLX + 19, curEY - 20, curLX + 19, curEY + 4, SSD1306_BLACK);
    display.fillTriangle(curRX + 19, curEY - 20, curRX - 19, curEY - 20, curRX - 19, curEY + 4, SSD1306_BLACK);

    display.drawLine(52 + ox, curEY - 8, 76 + ox, curEY - 8, SSD1306_WHITE);
    display.drawLine(56 + ox, curEY - 5, 72 + ox, curEY - 5, SSD1306_WHITE);

    display.fillCircle(curLX + 4, curEY + 5, 5, SSD1306_BLACK);
    display.fillCircle(curRX - 4, curEY + 5, 5, SSD1306_BLACK);

    display.drawRoundRect(52 + ox, 50 + oy, 24, 10, 2, SSD1306_WHITE);
    display.drawFastHLine(54 + ox, 55 + oy, 20, SSD1306_WHITE);
    display.drawFastVLine(60 + ox, 51 + oy, 8, SSD1306_WHITE);
    display.drawFastVLine(66 + ox, 51 + oy, 8, SSD1306_WHITE);
    display.drawFastVLine(72 + ox, 51 + oy, 8, SSD1306_WHITE);

    int veinSize = (r % 2 == 0) ? 9 : 5;
    drawAngerVein(112, 12, veinSize);

    // Audio rumble directly synchronized with screen vibration!
    if (r % 2 == 0) {
        tone(AUDIO_PIN, 95);  // Low growling rumble on shake left
    } else {
        tone(AUDIO_PIN, 145); // Electric buzz on shake right
    }
    if (r == 13) noTone(AUDIO_PIN);
}

// --------------------------------------------------------
// 4. 😍 IN LOVE (Heartbeat Sound "LUB-DUB" in Sync with Pulse)
// --------------------------------------------------------
void renderLoveFrame(int p) {
    display.clearDisplay();

    // Pulse progression: 0 -> 1 -> 2 -> 3 -> 4 -> 3 -> 2 -> 1 -> 0
    int sz = (p < 4) ? p : (7 - p);
    drawHeart(LX, EY, 24 + (sz * 3));
    drawHeart(RX, EY, 24 + (sz * 3));
    drawHeart(14, 16 + sz, 7 + sz);
    drawHeart(114, 16 + sz, 7 + sz);

    for (int i = 0; i < 2; i++) {
        display.drawCircle(64, 48 - i, 8, SSD1306_WHITE);
    }
    display.fillRect(52, 42, 24, 8, SSD1306_BLACK);
    display.fillCircle(LX, EY + 20, 2, SSD1306_WHITE);
    display.fillCircle(RX, EY + 20, 2, SSD1306_WHITE);

    // Real heartbeat audio synced to heart shape!
    if (p == 1) {
        tone(AUDIO_PIN, 85);  // "LUB" when starting expansion
    } else if (p == 3) {
        tone(AUDIO_PIN, 120); // "DUB" at maximum expansion peak!
    } else {
        noTone(AUDIO_PIN);    // Silence between beats
    }
}

// --------------------------------------------------------
// 5. 😱 SHOCKED (Ascending Frequency Sweep in Sync with Eyes)
// --------------------------------------------------------
void renderShockFrame(int s) {
    display.clearDisplay();
    int r = 18 + s; // Eyes expanding

    display.fillCircle(LX, EY, r, SSD1306_WHITE);
    display.fillCircle(RX, EY, r, SSD1306_WHITE);
    display.fillCircle(LX, EY, 7 + (s/2), SSD1306_BLACK);
    display.fillCircle(RX, EY, 7 + (s/2), SSD1306_BLACK);
    display.fillCircle(LX + 3, EY - 3, 3, SSD1306_WHITE);
    display.fillCircle(RX + 3, EY - 3, 3, SSD1306_WHITE);

    display.drawCircle(LX, EY - 24 - (s/2), 12, SSD1306_WHITE);
    display.fillRect(LX - 14, EY - 24 - (s/2), 28, 14, SSD1306_BLACK);
    display.drawCircle(RX, EY - 24 - (s/2), 12, SSD1306_WHITE);
    display.fillRect(RX - 14, EY - 24 - (s/2), 28, 14, SSD1306_BLACK);

    display.fillCircle(64, 52, 4 + s, SSD1306_WHITE);
    display.fillCircle(64, 52, 2 + (s/2), SSD1306_BLACK);

    display.drawLine(10, 8, 16, 14, SSD1306_WHITE);
    display.drawLine(4, 26, 12, 26, SSD1306_WHITE);
    display.drawLine(118, 8, 112, 14, SSD1306_WHITE);
    display.drawLine(124, 26, 116, 26, SSD1306_WHITE);

    // Audio sweep rises with eye & mouth expansion!
    tone(AUDIO_PIN, 900 + (s * 350));
    if (s >= 6) noTone(AUDIO_PIN);
}

// --------------------------------------------------------
// 6. 😴 SLEEPY (Rhythmic Breathing Tone in Sync with Zzz)
// --------------------------------------------------------
void renderSleepFrame(int z) {
    display.clearDisplay();

    for (int i = 0; i < 3; i++) {
        display.drawCircle(LX, EY - 4 + i, 15, SSD1306_WHITE);
        display.drawCircle(RX, EY - 4 + i, 15, SSD1306_WHITE);
    }
    display.fillRect(LX - 18, EY - 18, 36, 14, SSD1306_BLACK);
    display.fillRect(RX - 18, EY - 18, 36, 14, SSD1306_BLACK);
    display.drawCircle(64, 52, 2, SSD1306_WHITE);

    int y1 = 32 - (z % 24);
    int y2 = 22 - (z % 24);
    int y3 = 12 - (z % 24);
    display.setTextSize(1);
    if (y1 > 0) { display.setCursor(96, y1);  display.print(F("z")); }
    if (y2 > 0) { display.setCursor(106, y2); display.print(F("Z")); }
    if (y3 > 0) { display.setCursor(116, y3); display.print(F("Z")); }

    // Rhythmic breathing tone (low sine hum rise and fall)
    int breath = z % 12;
    if (breath < 6) {
        tone(AUDIO_PIN, 110 + (breath * 6)); // Inhale
    } else {
        tone(AUDIO_PIN, 146 - ((breath - 6) * 6)); // Exhale
    }
    if (z >= 20) noTone(AUDIO_PIN);
}

// --------------------------------------------------------
// SETUP & LOOP
// --------------------------------------------------------
void setup() {
    Serial.begin(9600);
    pinMode(AUDIO_PIN, OUTPUT);
    digitalWrite(AUDIO_PIN, LOW);
    delay(500);

    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F("   🎭 REAL-TIME SYNCED FACE & AUDIO       "));
    Serial.println(F("=========================================="));
    Serial.println(F("Every video frame is locked to sound:"));
    Serial.println(F("  'h' -> HAPPY (Pitch follows eye bounce)"));
    Serial.println(F("  's' -> SAD (Whine drops with tear + plink!)"));
    Serial.println(F("  'a' -> ANGRY (Rumble shakes with pixels)"));
    Serial.println(F("  'l' -> LOVE (Heartbeat on heart expansion)"));
    Serial.println(F("  'o' -> SHOCKED (Sweep matches dilating eyes)"));
    Serial.println(F("  'z' -> SLEEPY (Breathing hum with Zzz)"));
    Serial.println(F("  '0' -> Auto-Cycle All"));
    Serial.println(F("=========================================="));

    Wire.begin();
    delay(100);

    uint8_t address = 0;
    const uint8_t addrs[2] = {0x3C, 0x3D};
    for (int i = 0; i < 2; i++) {
        Wire.beginTransmission(addrs[i]);
        if (Wire.endTransmission() == 0) { address = addrs[i]; break; }
    }
    if (address == 0) address = 0x3C;

    display.begin(SSD1306_SWITCHCAPVCC, address);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    modeTimer = millis();
}

void loop() {
    // 1. Manual Serial Override
    if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'h' || c == '1') currentMode = 1;
        else if (c == 's' || c == '2') currentMode = 2;
        else if (c == 'a' || c == '3') currentMode = 3;
        else if (c == 'l' || c == '4') currentMode = 4;
        else if (c == 'o' || c == '5') currentMode = 5;
        else if (c == 'z' || c == '6') currentMode = 6;
        else if (c == '0') currentMode = 0;
        modeTimer = millis() + 5000;
        noTone(AUDIO_PIN);
    }

    // 2. Auto-Cycle Mode
    int activeMood = currentMode;
    if (activeMood == 0) {
        unsigned long now = millis();
        if (now >= modeTimer) {
            autoMood = (autoMood % 6) + 1;
            modeTimer = now + 4000; // Change emotion every 4 seconds
            noTone(AUDIO_PIN);
        }
        activeMood = autoMood;
    }

    // 3. Play Frame-Synchronized Animations & Audio
    switch (activeMood) {
        case 1: { // 😄 Happy
            for (int f = 0; f < 8; f++) {
                renderHappyFrame(f);
                display.display();
                delay(80);
            }
            noTone(AUDIO_PIN);
            delay(150);
            break;
        }
        case 2: { // 😭 Sad
            for (int t = 0; t < 12; t++) {
                renderSadFrame(t);
                display.display();
                delay(90);
            }
            noTone(AUDIO_PIN);
            delay(200);
            break;
        }
        case 3: { // 😡 Angry
            for (int r = 0; r < 14; r++) {
                renderAngryFrame(r);
                display.display();
                delay(60);
            }
            noTone(AUDIO_PIN);
            delay(150);
            break;
        }
        case 4: { // 😍 In Love
            for (int p = 0; p < 8; p++) {
                renderLoveFrame(p);
                display.display();
                delay(70);
            }
            noTone(AUDIO_PIN);
            delay(200);
            break;
        }
        case 5: { // 😱 Shocked
            for (int s = 0; s < 7; s++) {
                renderShockFrame(s);
                display.display();
                delay(70);
            }
            noTone(AUDIO_PIN);
            delay(300);
            break;
        }
        case 6: { // 😴 Sleepy
            for (int z = 0; z < 22; z++) {
                renderSleepFrame(z);
                display.display();
                delay(80);
            }
            noTone(AUDIO_PIN);
            delay(100);
            break;
        }
    }
}
