#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include "config.h"
#include "voice_samples.h"

// =========================================================================
// 🤖 ESP32 DESK BUDDY: ULTIMATE INTERACTIVE EMO-STYLE LIVING COMPANION
// =========================================================================
// Full Master Suite:
//   • Dual-Mode WiFi: Direct AP Hotspot ("DeskBuddy-WiFi") + Router Client (STA)
//   • Smart WiFi Provisioning: Scan 2.4GHz routers, connect, save to Flash (NVS)
//   • Google Gemini 1.5 Flash AI Brain: Talk via Web Voice, AI reacts with emotions!
//   • Microphone Sound Sensor: Single clap wakes up, double clap triggers Party Mode!
//   • Capacitive Petting: Stroke GPIO 4 to pet robot, purring cat face & wiggle!
//   • 21 Crystal-Clear 16kHz HD Studio Voice Tracks via Hardware DAC (GPIO 25)
//   • 18 Expressive OLED Face Animations + Biological Breathing & Saccades
//   • Playable Mini-Games: Flappy Bird on OLED, Rock Paper Scissors, Magic 8-Ball
//   • Live NTP Clock & Real-time Weather Sync via Open-Meteo
//   • Glassmorphic Mobile Web Companion Dashboard (http://deskbuddy.local)
// =========================================================================

// Subsystem Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo headServo;
WebServer server(80);
Preferences prefs;
bool oledReady = false;

// Persistent Settings
String staSSID = "";
String staPass = "";
String geminiKey = "";
int gmtOffsetHours = 0;
bool wifiStaConnected = false;
unsigned long nextWifiCheck = 0;
unsigned long nextWeatherCheck = 0;
String currentWeatherStr = "Sunny";
int currentTempC = 24;

// Eye coordinate anchors (EMO Binocular Geometry)
const int EYE_L_CX = 38;
const int EYE_R_CX = 90;
const int EYE_CY   = 24;
const int EYE_W    = 36;
const int EYE_H    = 40;
const int EYE_R    = 10;

// States
enum CompanionState {
    STATE_AWAKE_IDLE,
    STATE_CURIOUS_PERK,
    STATE_HAPPY_AFFECTION,
    STATE_DROWSY_NAP,
    STATE_DEEP_SLEEP,
    STATE_GAME_FLAPPY,
    STATE_GAME_RPS,
    STATE_SENTRY_GUARD,
    STATE_FOCUS_STUDY,
    STATE_SNACK_FEEDING,
    STATE_EXPRESSION_ACTION,
    STATE_AI_THINKING,
    STATE_AI_SPEAKING
};

CompanionState robotState = STATE_AWAKE_IDLE;

enum RobotEmotion {
    EMOTION_IDLE,
    EMOTION_HELLO,
    EMOTION_LOVE,
    EMOTION_TADA,
    EMOTION_PARTY_DJ,
    EMOTION_CURIOUS_SCAN,
    EMOTION_UHOH_ALERT,
    EMOTION_COOL_SUNGLASSES,
    EMOTION_SLEEP,
    EMOTION_GAMER_PACMAN,
    EMOTION_RAINY_SAD,
    EMOTION_FIRE_RAGE,
    EMOTION_HYPNO_DIZZY,
    EMOTION_KAWAII_CAT,
    EMOTION_JACKPOT_MONEY,
    EMOTION_MATRIX_HACKER,
    EMOTION_KAWAII_KISS,
    EMOTION_FOCUS_STUDY,
    EMOTION_MAGIC_8BALL,
    EMOTION_SENTRY_ALERT,
    EMOTION_SNACK_EAT,
    EMOTION_RPS_SHOW
};

RobotEmotion currentEmotion = EMOTION_IDLE;
unsigned long emotionResetTime = 0;
unsigned long lastInteractionTime = 0;
const char* magic8Answer = "YES! 100%";
const char* rpsRobotChoice = "ROCK";
String lastAIResponseText = "";

// Virtual Companion Stats (Tamagotchi Engine)
int affectionLevel = 80;
int energyLevel    = 100;
int hungerLevel    = 85;
unsigned long nextSpontaneousBehavior = 0;

// Flappy Game Engine Variables
int flappyBirdY = 28;
float flappyVel = 0.0f;
int flappyScore = 0;
int flappyPipeX = 120;
int flappyPipeGapY = 24;
bool flappyGameOver = false;
unsigned long nextFlappyTick = 0;

// Sentry Guard Variables
bool sentryActive = false;
float sentryPanAngle = SERVO_CENTER;
bool sentryPanDir = true;
unsigned long nextSentryPan = 0;

// Capacitive Touch Sensor
bool isBeingPetted = false;

// Microphone Sound & Clap Sensor Engine
int clapPulseCount = 0;
unsigned long firstClapPulseTime = 0;
unsigned long micMuteUntil = 0;

// Servo Motion State
float currentServoAngle = SERVO_CENTER;
float targetServoAngle  = SERVO_CENTER;
bool isWiggling         = false;
int wiggleStep          = 0;
unsigned long nextWiggleTime = 0;

// Gaze & Eye Saccades
float lookX = 0.0f;
float targetLookX = 0.0f;
float lookY = 0.0f;
float targetLookY = 0.0f;
bool isBlinking = false;
float blinkProgress = 0.0f;
bool doubleBlinkPending = false;
unsigned long nextBlinkTime = 0;
unsigned long nextGazeTime = 0;

// =========================================================================
// FORWARD DECLARATIONS
// =========================================================================
void playRobotVoiceHD(const uint8_t *audioData, int length, int mouthShape, const char* subtitle);
void robotSayHello();
void robotSayLove();
void robotSayTada();
void robotSayParty();
void robotSayCurious();
void robotSayUhOh();
void robotSaySleep();
void robotSayGame();
void robotSaySad();
void robotSayFire();
void robotSayDizzy();
void robotSayCat();
void robotSayMoney();
void robotSayHacker();
void robotSayKiss();
void robotSayFocus();
void robotSaySentry();
void robotSaySnack();
void robotSayRPS();
void robotSayWin();
void robotSayLose();

void triggerWiggle();
void updateServoMotion();
void startFlappyGame();
void jumpFlappyBird();
void updateFlappyGame();
void startRockPaperScissors();
void feedSnack(const char* snackName);

void renderLivingIdleFace(float gazeX, float gazeY, float openRatio);
void renderFlappyGame();
void renderRPS(const char* choice);
void renderSentryAlert(int tick);
void renderSnackEat(int chewFrame);
void renderEmoCoolSunglasses(int glintOffset);
void renderEmoPartyDJ(int step);
void renderWalleLoveFace(int pulse);
void renderCyberHUD(int scanY);
void renderSleepMode(int zStep);
void renderGamerPacman(int frame);
void renderKawaiiCat(int earTwitch);
void renderFocusStudy(int tick);
void renderRainySad(int dropStep);
void renderFireRage(int flameStep);
void renderHypnoDizzy(int rot);
void renderJackpotMoney(int coinStep);
void renderMatrixHacker(int frame);
void renderKawaiiKiss(int heartFlight);
void renderMagic8BallScreen(const char* answer);
void drawSpeechScreen(int mouthType, const char* subtitle);
void drawHeart(int cx, int cy, int size);
void displayScrollingMessage(const String &text, const char* title);

void handleRoot();
void handleCommand();
void handleStatus();
void handleWifiScan();
void handleWifiSave();
void handleGeminiKey();
void handleGeminiAsk();
void handleNotify();
void handleBillboard();

void checkMicrophoneClap();
void askGeminiAI(const String &prompt);
void updateInternetClockAndWeather();

// =========================================================================
// 🦾 SMOOTH SERVO CONTROLLER (Cubic Easing + Biological Lag)
// =========================================================================

void triggerWiggle() {
    isWiggling = true;
    wiggleStep = 0;
    nextWiggleTime = millis();
    micMuteUntil = millis() + 900; // Mute mic during wiggle
}

void updateServoMotion() {
    unsigned long now = millis();

    if (isWiggling && now >= nextWiggleTime) {
        switch (wiggleStep) {
            case 0: targetServoAngle = SERVO_CENTER - 22.0f; nextWiggleTime = now + 100; break;
            case 1: targetServoAngle = SERVO_CENTER + 22.0f; nextWiggleTime = now + 100; break;
            case 2: targetServoAngle = SERVO_CENTER - 18.0f; nextWiggleTime = now + 90; break;
            case 3: targetServoAngle = SERVO_CENTER + 18.0f; nextWiggleTime = now + 90; break;
            case 4: targetServoAngle = SERVO_CENTER - 10.0f; nextWiggleTime = now + 80; break;
            case 5: targetServoAngle = SERVO_CENTER + 10.0f; nextWiggleTime = now + 80; break;
            case 6: targetServoAngle = SERVO_CENTER;         nextWiggleTime = now + 80; break;
            default: isWiggling = false; break;
        }
        if (isWiggling) wiggleStep++;
    }

    float diff = targetServoAngle - currentServoAngle;
    if (fabs(diff) > 0.25f) {
        currentServoAngle += diff * SERVO_EASING;
        headServo.write((int)round(currentServoAngle));
        micMuteUntil = max(micMuteUntil, millis() + 150);
    }
}

// =========================================================================
// 🎨 PROCEDURAL VECTOR OLED EXPRESSIONS
// =========================================================================

void drawHeart(int cx, int cy, int size) {
    int r = size / 4;
    display.fillCircle(cx - r, cy - r, r, SSD1306_WHITE);
    display.fillCircle(cx + r, cy - r, r, SSD1306_WHITE);
    display.fillTriangle(cx - size/2, cy - r, cx + size/2, cy - r, cx, cy + size/2, SSD1306_WHITE);
}

// 1. Biological Living Idle
void renderLivingIdleFace(float gazeX, float gazeY, float openRatio) {
    if (!oledReady) return;
    display.clearDisplay();

    int offX = (int)(gazeX * 12.0f);
    int offY = (int)(gazeY * 8.0f);

    float breath = sin(millis() * 0.0022f);
    int hBreath = (int)(breath * 2.0f);
    int curH = max(2, (int)((EYE_H + hBreath) * openRatio));
    int curW = EYE_W;

    if (openRatio < 0.15f) {
        display.drawFastHLine(EYE_L_CX - EYE_W/2, EYE_CY, EYE_W, SSD1306_WHITE);
        display.drawFastHLine(EYE_R_CX - EYE_W/2, EYE_CY, EYE_W, SSD1306_WHITE);
        return;
    }

    int leftX = (EYE_L_CX + offX) - curW / 2;
    int leftY = (EYE_CY + offY) - curH / 2;
    display.fillRoundRect(leftX, leftY, curW, curH, min(EYE_R, curH/2), SSD1306_WHITE);

    int rightX = (EYE_R_CX + offX) - curW / 2;
    int rightY = (EYE_CY + offY) - curH / 2;
    display.fillRoundRect(rightX, rightY, curW, curH, min(EYE_R, curH/2), SSD1306_WHITE);

    if (curH > 14) {
        int pupilSize = 6;
        int glintSize = 2;
        int pLX = (EYE_L_CX + offX) + (int)(gazeX * 4.0f);
        int pLY = (EYE_CY + offY) + (int)(gazeY * 3.0f);
        display.fillCircle(pLX, pLY, pupilSize, SSD1306_BLACK);
        display.fillCircle(pLX + 2, pLY - 2, glintSize, SSD1306_WHITE);

        int pRX = (EYE_R_CX + offX) + (int)(gazeX * 4.0f);
        int pRY = (EYE_CY + offY) + (int)(gazeY * 3.0f);
        display.fillCircle(pRX, pRY, pupilSize, SSD1306_BLACK);
        display.fillCircle(pRX + 2, pRY - 2, glintSize, SSD1306_WHITE);
    }
}

// 2. Playable Flappy Game
void renderFlappyGame() {
    if (!oledReady) return;
    display.clearDisplay();

    display.fillRoundRect(18, flappyBirdY, 14, 10, 3, SSD1306_WHITE);
    display.fillCircle(28, flappyBirdY + 3, 2, SSD1306_BLACK);
    display.fillTriangle(32, flappyBirdY + 5, 36, flappyBirdY + 7, 32, flappyBirdY + 9, SSD1306_WHITE);
    int wingY = (flappyVel < 0) ? flappyBirdY + 6 : flappyBirdY + 1;
    display.drawLine(20, flappyBirdY + 5, 14, wingY, SSD1306_WHITE);

    display.fillRect(flappyPipeX, 0, 14, flappyPipeGapY, SSD1306_WHITE);
    display.fillRect(flappyPipeX - 2, flappyPipeGapY - 4, 18, 4, SSD1306_WHITE);
    int bottomPipeY = flappyPipeGapY + 28;
    display.fillRect(flappyPipeX, bottomPipeY, 14, 64 - bottomPipeY, SSD1306_WHITE);
    display.fillRect(flappyPipeX - 2, bottomPipeY, 18, 4, SSD1306_WHITE);

    display.drawFastHLine(0, 63, 128, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(4, 2);
    display.print(F("SCORE: "));
    display.print(flappyScore);

    if (flappyGameOver) {
        display.fillRoundRect(20, 20, 88, 26, 4, SSD1306_BLACK);
        display.drawRoundRect(20, 20, 88, 26, 4, SSD1306_WHITE);
        display.setCursor(28, 24);
        display.print(F("GAME OVER!"));
        display.setCursor(24, 34);
        display.print(F("TAP TO RESTART"));
    }
}

// 3. Rock Paper Scissors
void renderRPS(const char* choice) {
    if (!oledReady) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(14, 4);
    display.print(F("DESK BUDDY CHOSE:"));

    if (strcmp(choice, "ROCK") == 0) {
        display.fillCircle(64, 38, 16, SSD1306_WHITE);
        display.drawCircle(64, 38, 18, SSD1306_WHITE);
    } else if (strcmp(choice, "PAPER") == 0) {
        display.fillRect(48, 22, 32, 32, SSD1306_WHITE);
        display.fillRect(52, 26, 24, 24, SSD1306_BLACK);
    } else {
        display.drawLine(46, 22, 82, 54, SSD1306_WHITE);
        display.drawLine(46, 23, 82, 55, SSD1306_WHITE);
        display.drawLine(46, 54, 82, 22, SSD1306_WHITE);
        display.drawLine(46, 55, 82, 23, SSD1306_WHITE);
        display.drawCircle(46, 22, 6, SSD1306_WHITE);
        display.drawCircle(46, 54, 6, SSD1306_WHITE);
    }

    display.setTextSize(1);
    int len = strlen(choice);
    display.setCursor((128 - (len * 6)) / 2, 56);
    display.print(choice);
}

// 4. Sentry Alert Mode
void renderSentryAlert(int tick) {
    if (!oledReady) return;
    display.clearDisplay();
    display.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);

    int scanX = 16 + (tick % 96);
    display.drawFastVLine(scanX, 0, 64, SSD1306_BLACK);
    display.drawFastVLine(scanX + 1, 0, 64, SSD1306_BLACK);

    display.drawTriangle(EYE_L_CX - 18, 4, EYE_L_CX + 18, 4, EYE_L_CX + 18, 20, SSD1306_BLACK);
    display.drawTriangle(EYE_R_CX - 18, 4, EYE_R_CX + 18, 4, EYE_R_CX - 18, 20, SSD1306_BLACK);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 54);
    display.print(F("! SENTRY ACTIVE !"));
}

// 5. Snack Eating Animation
void renderSnackEat(int chewFrame) {
    if (!oledReady) return;
    display.clearDisplay();
    int chew = (chewFrame % 4 < 2) ? 6 : 0;

    display.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - 12 + chew/2, EYE_W, 24 - chew, 8, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - 12 + chew/2, EYE_W, 24 - chew, 8, SSD1306_WHITE);

    display.fillTriangle(54, 42, 74, 42, 64, 60, SSD1306_WHITE);
    display.fillCircle(64, 48, 2, SSD1306_BLACK);
    display.fillCircle(60, 45, 1, SSD1306_BLACK);
    display.fillCircle(68, 45, 1, SSD1306_BLACK);

    display.setTextSize(1);
    display.setCursor(34, 4);
    display.print(F("YUM YUM! 🍕"));
}

// 6. Cool Sunglasses
void renderEmoCoolSunglasses(int glintOffset) {
    if (!oledReady) return;
    display.clearDisplay();
    int frameW = 48;
    int frameH = 34;

    display.fillRoundRect(EYE_L_CX - frameW/2, EYE_CY - frameH/2, frameW, frameH, 6, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - frameW/2, EYE_CY - frameH/2, frameW, frameH, 6, SSD1306_WHITE);
    display.fillRect(EYE_L_CX + frameW/2 - 4, EYE_CY - 10, 16, 6, SSD1306_WHITE);

    int g1 = (glintOffset % 50) - 10;
    display.drawLine(EYE_L_CX - 18 + g1, EYE_CY - 14, EYE_L_CX - 6 + g1, EYE_CY + 14, SSD1306_BLACK);
    display.drawLine(EYE_L_CX - 14 + g1, EYE_CY - 14, EYE_L_CX - 2 + g1, EYE_CY + 14, SSD1306_BLACK);
    display.drawLine(EYE_R_CX - 18 + g1, EYE_CY - 14, EYE_R_CX - 6 + g1, EYE_CY + 14, SSD1306_BLACK);
    display.drawLine(EYE_R_CX - 14 + g1, EYE_CY - 14, EYE_R_CX - 2 + g1, EYE_CY + 14, SSD1306_BLACK);

    display.drawLine(56, 52, 72, 48, SSD1306_WHITE);
    display.drawLine(72, 48, 76, 44, SSD1306_WHITE);
}

// 7. Party DJ
void renderEmoPartyDJ(int step) {
    if (!oledReady) return;
    display.clearDisplay();

    for (int i = 0; i < 4; i++) {
        int barH = 8 + (int)(18 * fabs(sin((step * 0.3) + i * 1.2)));
        display.fillRect(16 + i * 8, 48 - barH, 6, barH, SSD1306_WHITE);
    }
    for (int i = 0; i < 4; i++) {
        int barH = 8 + (int)(18 * fabs(cos((step * 0.3) + i * 1.2)));
        display.fillRect(80 + i * 8, 48 - barH, 6, barH, SSD1306_WHITE);
    }

    display.drawCircle(EYE_L_CX, EYE_CY - 4, 14, SSD1306_WHITE);
    display.drawCircle(EYE_R_CX, EYE_CY - 4, 14, SSD1306_WHITE);
    display.drawArc(64, 16, 36, 34, 0, 180, SSD1306_WHITE);

    display.drawCircle(64, 52, 6, SSD1306_WHITE);
    display.fillRect(56, 46, 16, 6, SSD1306_BLACK);
}

// 8. Love Heart Face
void renderWalleLoveFace(int pulse) {
    if (!oledReady) return;
    display.clearDisplay();
    drawHeart(EYE_L_CX, EYE_CY, 24 + pulse);
    drawHeart(EYE_R_CX, EYE_CY, 24 + pulse);
    drawHeart(14, 16 + pulse, 8 + pulse);
    drawHeart(114, 16 + pulse, 8 + pulse);
    display.drawCircle(64, 48, 6, SSD1306_WHITE);
    display.fillRect(56, 42, 16, 6, SSD1306_BLACK);
}

// 9. Cyber HUD Scan
void renderCyberHUD(int scanY) {
    if (!oledReady) return;
    display.clearDisplay();

    display.drawRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    display.drawRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);

    int sy = scanY % 64;
    display.drawFastHLine(0, sy, 128, SSD1306_WHITE);
    display.drawFastHLine(0, (sy + 2) % 64, 128, SSD1306_WHITE);

    display.drawCircle(EYE_L_CX, EYE_CY, 8, SSD1306_WHITE);
    display.drawCircle(EYE_R_CX, EYE_CY, 8, SSD1306_WHITE);
    display.drawFastVLine(EYE_L_CX, EYE_CY - 12, 24, SSD1306_WHITE);
    display.drawFastHLine(EYE_L_CX - 12, EYE_CY, 24, SSD1306_WHITE);
    display.drawFastVLine(EYE_R_CX, EYE_CY - 12, 24, SSD1306_WHITE);
    display.drawFastHLine(EYE_R_CX - 12, EYE_CY, 24, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(18, 54);
    display.print(F("SCANNING TARGET..."));
}

// 10. Sleep Mode
void renderSleepMode(int zStep) {
    if (!oledReady) return;
    display.clearDisplay();

    display.drawFastHLine(EYE_L_CX - 16, EYE_CY + 4, 32, SSD1306_WHITE);
    display.drawFastHLine(EYE_L_CX - 14, EYE_CY + 5, 28, SSD1306_WHITE);
    display.drawFastHLine(EYE_R_CX - 16, EYE_CY + 4, 32, SSD1306_WHITE);
    display.drawFastHLine(EYE_R_CX - 14, EYE_CY + 5, 28, SSD1306_WHITE);

    int zOff = (zStep % 28);
    display.setTextSize(1);
    display.setCursor(96 + (zOff / 4), 30 - zOff);
    display.print(F("z"));
    display.setTextSize(2);
    display.setCursor(106 + (zOff / 3), 22 - zOff);
    display.print(F("Z"));

    display.drawCircle(64, 52, 4, SSD1306_WHITE);
}

// 11. Gamer Pacman
void renderGamerPacman(int frame) {
    if (!oledReady) return;
    display.clearDisplay();
    int px = (frame * 4) % 140 - 20;

    display.fillCircle(px, EYE_CY, 18, SSD1306_WHITE);
    int mouthOpen = (frame % 4 < 2) ? 14 : 4;
    display.fillTriangle(px, EYE_CY, px + 20, EYE_CY - mouthOpen, px + 20, EYE_CY + mouthOpen, SSD1306_BLACK);

    for (int dot = px + 28; dot < 128; dot += 18) {
        display.fillCircle(dot, EYE_CY, 3, SSD1306_WHITE);
    }

    int gx = px - 32;
    display.fillRoundRect(gx, EYE_CY - 16, 24, 32, 10, SSD1306_WHITE);
    int footShift = (frame % 2 == 0) ? 0 : 2;
    display.fillRect(gx + 2 + footShift, gy + 22, 5, 4, SSD1306_BLACK);
    display.fillRect(gx + 11 + footShift, gy + 22, 5, 4, SSD1306_BLACK);
    display.fillRect(gx + 19 - footShift, gy + 22, 5, 4, SSD1306_BLACK);
    display.fillCircle(gx + 7, gy + 8, 3, SSD1306_BLACK);
    display.fillCircle(gx + 17, gy + 8, 3, SSD1306_BLACK);

    display.setTextSize(1);
    display.setCursor(18, 54);
    display.print(F("LEVEL UP! [ 1UP ]"));
}

// 12. Kawaii Cat
void renderKawaiiCat(int earTwitch) {
    if (!oledReady) return;
    display.clearDisplay();
    int et = (earTwitch % 6 < 2) ? 2 : 0;
    display.fillTriangle(EYE_L_CX - 16, 12, EYE_L_CX - 6, 2 - et, EYE_L_CX + 4, 12, SSD1306_WHITE);
    display.fillTriangle(EYE_R_CX - 4, 12, EYE_R_CX + 6, 2 + et, EYE_R_CX + 16, 12, SSD1306_WHITE);

    display.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - 14, EYE_W, 32, 10, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - 14, EYE_W, 32, 10, SSD1306_WHITE);
    display.fillCircle(EYE_L_CX, EYE_CY, 8, SSD1306_BLACK);
    display.fillCircle(EYE_R_CX, EYE_CY, 8, SSD1306_BLACK);
    display.fillCircle(EYE_L_CX + 3, EYE_CY - 3, 3, SSD1306_WHITE);
    display.fillCircle(EYE_R_CX + 3, EYE_CY - 3, 3, SSD1306_WHITE);

    display.drawLine(8, 22, 18, 24, SSD1306_WHITE);
    display.drawLine(8, 28, 18, 26, SSD1306_WHITE);
    display.drawLine(120, 22, 110, 24, SSD1306_WHITE);
    display.drawLine(120, 28, 110, 26, SSD1306_WHITE);

    display.drawCircle(60, 48, 3, SSD1306_WHITE);
    display.fillRect(57, 45, 6, 3, SSD1306_BLACK);
    display.drawCircle(66, 48, 3, SSD1306_WHITE);
    display.fillRect(66, 45, 6, 3, SSD1306_BLACK);
}

// 13. Focus Study
void renderFocusStudy(int tick) {
    if (!oledReady) return;
    display.clearDisplay();
    display.drawCircle(EYE_L_CX, EYE_CY, 16, SSD1306_WHITE);
    display.drawCircle(EYE_R_CX, EYE_CY, 16, SSD1306_WHITE);
    display.drawLine(EYE_L_CX + 16, EYE_CY - 4, EYE_R_CX - 16, EYE_CY - 4, SSD1306_WHITE);

    display.fillCircle(EYE_L_CX, EYE_CY, 5, SSD1306_WHITE);
    display.fillCircle(EYE_R_CX, EYE_CY, 5, SSD1306_WHITE);

    display.drawRoundRect(20, 48, 88, 12, 3, SSD1306_WHITE);
    int bar = 4 + (tick % 80);
    display.fillRect(22, 50, bar, 8, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(24, 2);
    display.print(F("FOCUS MODE [ 25:00 ]"));
}

// 14. Rainy Sad
void renderRainySad(int dropStep) {
    if (!oledReady) return;
    display.clearDisplay();

    display.fillCircle(50, 8, 6, SSD1306_WHITE);
    display.fillCircle(64, 6, 8, SSD1306_WHITE);
    display.fillCircle(78, 8, 6, SSD1306_WHITE);
    display.fillRoundRect(42, 8, 44, 6, 3, SSD1306_WHITE);

    int dy = 16 + (dropStep % 36);
    display.drawLine(48, dy, 48, dy + 3, SSD1306_WHITE);
    display.drawLine(64, (dy + 12) % 36 + 16, 64, (dy + 12) % 36 + 19, SSD1306_WHITE);
    display.drawLine(80, (dy + 24) % 36 + 16, 80, (dy + 24) % 36 + 27, SSD1306_WHITE);

    display.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - 10, EYE_W, 26, 8, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - 10, EYE_W, 26, 8, SSD1306_WHITE);
    display.fillTriangle(EYE_L_CX - 18, EYE_CY - 14, EYE_L_CX + 18, EYE_CY - 14, EYE_L_CX - 18, EYE_CY - 2, SSD1306_BLACK);
    display.fillTriangle(EYE_R_CX - 18, EYE_CY - 14, EYE_R_CX + 18, EYE_CY - 14, EYE_R_CX + 18, EYE_CY - 2, SSD1306_BLACK);

    display.fillCircle(EYE_L_CX - 8, EYE_CY + 18 + (dropStep % 10), 2, SSD1306_WHITE);
    display.fillCircle(EYE_R_CX + 8, EYE_CY + 18 + (dropStep % 10), 2, SSD1306_WHITE);

    display.drawCircle(64, 56, 4, SSD1306_WHITE);
    display.fillRect(58, 56, 12, 6, SSD1306_BLACK);
}

// 15. Fire Rage
void renderFireRage(int flameStep) {
    if (!oledReady) return;
    display.clearDisplay();

    for (int i = 0; i < 3; i++) {
        int fh1 = 6 + (int)(10 * fabs(sin((flameStep * 0.4) + i)));
        int fh2 = 6 + (int)(10 * fabs(cos((flameStep * 0.4) + i)));
        display.fillTriangle(EYE_L_CX - 12 + i*10, EYE_CY - 16, EYE_L_CX - 8 + i*10, EYE_CY - 16 - fh1, EYE_L_CX - 4 + i*10, EYE_CY - 16, SSD1306_WHITE);
        display.fillTriangle(EYE_R_CX - 12 + i*10, EYE_CY - 16, EYE_R_CX - 8 + i*10, EYE_CY - 16 - fh2, EYE_R_CX - 4 + i*10, EYE_CY - 16, SSD1306_WHITE);
    }

    display.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    display.fillTriangle(EYE_L_CX - 20, 2, EYE_L_CX + 20, 2, EYE_L_CX + 20, 20, SSD1306_BLACK);
    display.fillTriangle(EYE_R_CX - 20, 2, EYE_R_CX + 20, 2, EYE_R_CX - 20, 20, SSD1306_BLACK);

    display.fillRect(EYE_L_CX - 3, EYE_CY - 2, 6, 12, SSD1306_BLACK);
    display.fillRect(EYE_R_CX - 3, EYE_CY - 2, 6, 12, SSD1306_BLACK);

    display.drawRect(52, 48, 24, 6, SSD1306_WHITE);
    for (int x = 56; x < 76; x += 4) display.drawFastVLine(x, 48, 6, SSD1306_WHITE);
}

// 16. Hypno Dizzy
void renderHypnoDizzy(int rot) {
    if (!oledReady) return;
    display.clearDisplay();

    for (int r = 16; r > 3; r -= 5) {
        int offset = (rot + r * 2) % 8;
        display.drawRoundRect(EYE_L_CX - r, EYE_CY - r, r*2, r*2, offset, SSD1306_WHITE);
        display.drawRoundRect(EYE_R_CX - r, EYE_CY - r, r*2, r*2, offset, SSD1306_WHITE);
    }

    int sx1 = 64 + (int)(32 * cos(rot * 0.15));
    int sy1 = 24 + (int)(16 * sin(rot * 0.15));
    display.drawPixel(sx1, sy1, SSD1306_WHITE);
    display.drawPixel(sx1+1, sy1, SSD1306_WHITE);

    display.drawCircle(58, 50, 4, SSD1306_WHITE);
    display.fillRect(54, 46, 8, 4, SSD1306_BLACK);
    display.drawCircle(66, 50, 4, SSD1306_WHITE);
    display.fillRect(66, 50, 8, 4, SSD1306_BLACK);
}

// 17. Jackpot Money
void renderJackpotMoney(int coinStep) {
    if (!oledReady) return;
    display.clearDisplay();

    display.drawRoundRect(EYE_L_CX - 18, EYE_CY - 18, 36, 36, 4, SSD1306_WHITE);
    display.drawRoundRect(EYE_R_CX - 18, EYE_CY - 18, 36, 36, 4, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(EYE_L_CX - 6, EYE_CY - 7);
    display.print(F("$"));
    display.setCursor(EYE_R_CX - 6, EYE_CY - 7);
    display.print(F("$"));

    int cy = 4 + (coinStep % 54);
    display.fillCircle(12, cy, 3, SSD1306_WHITE);
    display.fillCircle(116, (cy + 18) % 54 + 4, 3, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(24, 52);
    display.print(F("JACKPOT! $$$"));
}

// 18. Matrix Hacker
void renderMatrixHacker(int frame) {
    if (!oledReady) return;
    display.clearDisplay();

    for (int col = 6; col < 124; col += 12) {
        int streamY = ((frame * 3) + col * 7) % 50;
        display.drawFastVLine(col, streamY, 8, SSD1306_WHITE);
        display.drawPixel(col, streamY + 10, SSD1306_WHITE);
    }

    display.drawRect(34, 16, 60, 32, SSD1306_WHITE);
    display.drawFastHLine(30, 32, 68, SSD1306_WHITE);
    display.drawFastVLine(64, 12, 40, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(22, 54);
    display.print(F("[ ACCESS GRANTED ]"));
}

// 19. Kawaii Kiss
void renderKawaiiKiss(int heartFlight) {
    if (!oledReady) return;
    display.clearDisplay();

    display.drawCircle(EYE_L_CX, EYE_CY + 4, 14, SSD1306_WHITE);
    display.fillRect(EYE_L_CX - 16, EYE_CY + 4, 32, 16, SSD1306_BLACK);

    display.fillCircle(EYE_R_CX, EYE_CY, 14, SSD1306_WHITE);
    display.fillCircle(EYE_R_CX, EYE_CY, 6, SSD1306_BLACK);
    display.fillCircle(EYE_R_CX + 2, EYE_CY - 2, 2, SSD1306_WHITE);

    int hx = 64 + (heartFlight % 50);
    int hy = 44 - (heartFlight % 32);
    drawHeart(hx, hy, 12);

    display.drawCircle(64, 50, 6, SSD1306_WHITE);
    display.fillRect(54, 44, 20, 6, SSD1306_BLACK);
}

// 20. Magic 8-Ball
void renderMagic8BallScreen(const char* answer) {
    if (!oledReady) return;
    display.clearDisplay();

    display.drawCircle(64, 30, 24, SSD1306_WHITE);
    display.fillCircle(64, 30, 14, SSD1306_WHITE);
    display.fillTriangle(54, 36, 74, 36, 64, 20, SSD1306_BLACK);

    display.setTextSize(1);
    int len = strlen(answer);
    int sx = (128 - (len * 6)) / 2;
    display.setCursor(max(4, sx), 56);
    display.print(answer);
}

void drawSpeechScreen(int mouthType, const char* subtitle) {
    if (!oledReady) return;
    display.clearDisplay();
    display.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    display.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    display.fillCircle(EYE_L_CX, EYE_CY, 6, SSD1306_BLACK);
    display.fillCircle(EYE_R_CX, EYE_CY, 6, SSD1306_BLACK);
    display.fillCircle(EYE_L_CX + 2, EYE_CY - 2, 2, SSD1306_WHITE);
    display.fillCircle(EYE_R_CX + 2, EYE_CY - 2, 2, SSD1306_WHITE);

    switch (mouthType) {
        case 0: display.drawFastHLine(56, 48, 16, SSD1306_WHITE); break;
        case 1: display.drawCircle(64, 48, 5, SSD1306_WHITE); display.fillCircle(64, 48, 2, SSD1306_BLACK); break;
        case 2: display.fillRoundRect(56, 45, 16, 8, 3, SSD1306_WHITE); display.fillRoundRect(58, 46, 12, 6, 2, SSD1306_BLACK); break;
        case 3: display.fillRoundRect(52, 44, 24, 12, 5, SSD1306_WHITE); display.fillCircle(64, 46, 5, SSD1306_BLACK); display.fillCircle(64, 53, 3, SSD1306_WHITE); break;
        case 4: default: display.drawCircle(64, 46, 8, SSD1306_WHITE); display.fillRect(52, 40, 24, 8, SSD1306_BLACK); break;
    }

    if (subtitle != NULL) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        int len = strlen(subtitle);
        int sx = (128 - (len * 6)) / 2;
        display.setCursor(max(4, sx), 56);
        display.print(subtitle);
    }
    display.display();
}

void displayScrollingMessage(const String &text, const char* title) {
    if (!oledReady) return;
    int strWidth = text.length() * 6;
    for (int x = 128; x > -strWidth; x -= 4) {
        display.clearDisplay();
        
        // Mini Eyes on top
        display.fillRoundRect(EYE_L_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
        display.fillRoundRect(EYE_R_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
        display.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
        display.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);
        
        if (title != NULL) {
            display.setTextSize(1);
            display.setCursor(4, 28);
            display.print(title);
        }
        
        display.drawRoundRect(2, 38, 124, 24, 4, SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(x, 46);
        display.print(text);
        display.display();
        delay(25);
    }
}

// =========================================================================
// 🔊 16kHz HD AUDIO ENGINE
// =========================================================================

void playRobotVoiceHD(const uint8_t *audioData, int length, int mouthShape, const char* subtitle) {
    micMuteUntil = millis() + (length / (VOICE_SAMPLE_RATE / 1000)) + 300; // Mute mic during speech

    if (subtitle != NULL) {
        drawSpeechScreen(mouthShape, subtitle);
    }

    int delayMicros = 1000000 / VOICE_SAMPLE_RATE;
    unsigned long nextSampleTime = micros();

    for (int i = 0; i < length; i++) {
        uint8_t raw = pgm_read_byte(&audioData[i]);

        int centered = (int)raw - 128;
        int scaled = 45 + ((centered * 70) / 128);
        if (scaled < 0) scaled = 0;
        if (scaled > 90) scaled = 90;

        while ((long)(micros() - nextSampleTime) < 0) {
            // zero jitter wait
        }
        nextSampleTime += delayMicros;

        dacWrite(AUDIO_DAC_PIN, (uint8_t)scaled);
    }

    for (int v = 45; v >= 0; v -= 3) {
        dacWrite(AUDIO_DAC_PIN, v);
        delayMicroseconds(400);
    }
    dacWrite(AUDIO_DAC_PIN, 0);

    drawSpeechScreen(4, subtitle);
    delay(150);
}

// -------------------------------------------------------------------------
// 🎭 SPOKEN COMPANION ACTIONS
// -------------------------------------------------------------------------

void robotSayHello() {
    targetServoAngle = SERVO_CENTER + 16.0f;
    playRobotVoiceHD(voice_hello_data, sizeof(voice_hello_data), 2, "\"Hello! I'm Desk Buddy!\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayLove() {
    targetServoAngle = SERVO_CENTER - 20.0f;
    playRobotVoiceHD(voice_love_data, sizeof(voice_love_data), 4, "\"I Love You! <3\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayTada() {
    targetServoAngle = SERVO_CENTER + 14.0f;
    playRobotVoiceHD(voice_tada_data, sizeof(voice_tada_data), 3, "\"Ta-Daaa! Ready! :D\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayParty() {
    triggerWiggle();
    playRobotVoiceHD(voice_party_data, sizeof(voice_party_data), 3, "\"Let's Dance & Party! \\m/\"");
}

void robotSayCurious() {
    targetServoAngle = SERVO_CENTER + 22.0f;
    playRobotVoiceHD(voice_curious_data, sizeof(voice_curious_data), 1, "\"Whoa! What is that? :O\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayUhOh() {
    targetServoAngle = SERVO_CENTER - 15.0f;
    playRobotVoiceHD(voice_uhoh_data, sizeof(voice_uhoh_data), 1, "\"Uh-Oh! Be careful! :(!\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSaySleep() {
    targetServoAngle = SERVO_CENTER - 25.0f;
    playRobotVoiceHD(voice_sleep_data, sizeof(voice_sleep_data), 0, "\"Good Night! Zzz...\"");
    targetServoAngle = SERVO_CENTER - 20.0f;
}

void robotSayGame() {
    targetServoAngle = SERVO_CENTER + 15.0f;
    playRobotVoiceHD(voice_game_data, sizeof(voice_game_data), 3, "\"Level Up! Game On!\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSaySad() {
    targetServoAngle = SERVO_CENTER - 16.0f;
    playRobotVoiceHD(voice_sad_data, sizeof(voice_sad_data), 0, "\"Don't Be Sad! <3\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayFire() {
    triggerWiggle();
    playRobotVoiceHD(voice_fire_data, sizeof(voice_fire_data), 3, "\"POWER LEVEL 9000! 🔥\"");
}

void robotSayDizzy() {
    targetServoAngle = SERVO_CENTER + 25.0f;
    playRobotVoiceHD(voice_dizzy_data, sizeof(voice_dizzy_data), 1, "\"Head is Spinning! @__@\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayCat() {
    targetServoAngle = SERVO_CENTER + 18.0f;
    playRobotVoiceHD(voice_cat_data, sizeof(voice_cat_data), 4, "\"Nya! Meow Meow! =^..^=\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayMoney() {
    targetServoAngle = SERVO_CENTER + 18.0f;
    playRobotVoiceHD(voice_money_data, sizeof(voice_money_data), 3, "\"JACKPOT! Cha-Ching! $$$\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayHacker() {
    targetServoAngle = SERVO_CENTER;
    playRobotVoiceHD(voice_hacker_data, sizeof(voice_hacker_data), 2, "\"ACCESS GRANTED! [SECURE]\"");
}

void robotSayKiss() {
    targetServoAngle = SERVO_CENTER - 15.0f;
    playRobotVoiceHD(voice_kiss_data, sizeof(voice_kiss_data), 4, "\"Mwah! You're Awesome!\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayFocus() {
    targetServoAngle = SERVO_CENTER;
    playRobotVoiceHD(voice_focus_data, sizeof(voice_focus_data), 2, "\"Focus Mode Active! 👓\"");
}

void robotSaySentry() {
    triggerWiggle();
    playRobotVoiceHD(voice_sentry_data, sizeof(voice_sentry_data), 1, "\"INTRUDER DETECTED! 🚨\"");
}

void robotSaySnack() {
    targetServoAngle = SERVO_CENTER + 12.0f;
    playRobotVoiceHD(voice_snack_data, sizeof(voice_snack_data), 3, "\"YUM YUM! Thank You! 🍕\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayRPS() {
    targetServoAngle = SERVO_CENTER + 14.0f;
    playRobotVoiceHD(voice_rps_data, sizeof(voice_rps_data), 3, "\"1, 2, 3... SHOOT!\"");
    targetServoAngle = SERVO_CENTER;
}

void robotSayWin() {
    triggerWiggle();
    playRobotVoiceHD(voice_win_data, sizeof(voice_win_data), 3, "\"YAY! YOU WIN! \\o/\"");
}

void robotSayLose() {
    targetServoAngle = SERVO_CENTER - 12.0f;
    playRobotVoiceHD(voice_lose_data, sizeof(voice_lose_data), 4, "\"Haha! I win! :P\"");
    targetServoAngle = SERVO_CENTER;
}

// =========================================================================
// 🎮 PLAYABLE MINI-GAMES & INTERACTIVE MODES
// =========================================================================

void startFlappyGame() {
    robotState = STATE_GAME_FLAPPY;
    flappyBirdY = 28;
    flappyVel = 0.0f;
    flappyScore = 0;
    flappyPipeX = 120;
    flappyPipeGapY = random(12, 30);
    flappyGameOver = false;
    nextFlappyTick = millis();
    robotSayGame();
}

void jumpFlappyBird() {
    if (flappyGameOver) {
        startFlappyGame();
    } else {
        flappyVel = -3.2f;
    }
}

void updateFlappyGame() {
    unsigned long now = millis();
    if (now < nextFlappyTick) return;
    nextFlappyTick = now + 45;

    if (!flappyGameOver) {
        flappyVel += 0.45f;
        flappyBirdY += (int)flappyVel;

        flappyPipeX -= 3;
        if (flappyPipeX < -16) {
            flappyPipeX = 128;
            flappyPipeGapY = random(10, 32);
            flappyScore++;
        }

        if (flappyBirdY > 50 || flappyBirdY < 0) {
            flappyGameOver = true;
            robotSayLose();
        }
        if (flappyPipeX >= 10 && flappyPipeX <= 36) {
            if (flappyBirdY < flappyPipeGapY || flappyBirdY + 10 > flappyPipeGapY + 28) {
                flappyGameOver = true;
                robotSayLose();
            }
        }
    }

    renderFlappyGame();
    display.display();
}

void startRockPaperScissors() {
    robotState = STATE_GAME_RPS;
    robotSayRPS();
    int r = random(0, 3);
    if (r == 0) rpsRobotChoice = "ROCK";
    else if (r == 1) rpsRobotChoice = "PAPER";
    else rpsRobotChoice = "SCISSORS";

    renderRPS(rpsRobotChoice);
    display.display();
    emotionResetTime = millis() + 5000;
}

void feedSnack(const char* snackName) {
    robotState = STATE_SNACK_FEEDING;
    currentEmotion = EMOTION_SNACK_EAT;
    hungerLevel = min(100, hungerLevel + 35);
    affectionLevel = min(100, affectionLevel + 15);
    robotSaySnack();
    emotionResetTime = millis() + 4500;
}

// =========================================================================
// 🎤 MICROPHONE SOUND & CLAP DETECTION ENGINE
// =========================================================================

void checkMicrophoneClap() {
#if ENABLE_SOUND_SENSOR
    unsigned long now = millis();
    if (now < micMuteUntil) return; // Muted during servo motion / audio playback

    int micVal = digitalRead(MIC_DO_PIN);
    // Typical sound sensor module pulls DO LOW on sound trigger
    if (micVal == LOW) {
        if (clapPulseCount == 0) {
            clapPulseCount = 1;
            firstClapPulseTime = now;
        } else if (clapPulseCount == 1 && (now - firstClapPulseTime > 120)) {
            // Double clap detected! (Within 120ms - 550ms)
            clapPulseCount = 0;
            lastInteractionTime = now;
            Serial.println(F("[Mic] 👏👏 DOUBLE CLAP DETECTED! Triggering Party Mode!"));
            currentEmotion = EMOTION_PARTY_DJ;
            robotSayParty();
            emotionResetTime = now + 6000;
            return;
        }
        micMuteUntil = now + 100; // Debounce
    }

    // Check single clap timeout
    if (clapPulseCount == 1 && (now - firstClapPulseTime > 550)) {
        clapPulseCount = 0;
        lastInteractionTime = now;
        Serial.println(F("[Mic] 👏 SINGLE CLAP DETECTED! Waking / Greeting!"));

        if (robotState == STATE_DEEP_SLEEP || robotState == STATE_DROWSY_NAP) {
            robotState = STATE_AWAKE_IDLE;
            currentEmotion = EMOTION_HELLO;
            robotSayHello();
            emotionResetTime = now + 4000;
        } else {
            // Wake perk
            currentEmotion = EMOTION_CURIOUS_SCAN;
            targetServoAngle = (targetServoAngle > SERVO_CENTER) ? SERVO_CENTER - 18.0f : SERVO_CENTER + 18.0f;
            robotSayCurious();
            emotionResetTime = now + 3500;
        }
    }
#endif
}

// =========================================================================
// 🧠 GOOGLE GEMINI AI BRAIN ENGINE (HTTPS REST API)
// =========================================================================

void askGeminiAI(const String &userPrompt) {
    Serial.print(F("[Gemini AI] Query: "));
    Serial.println(userPrompt);

    if (geminiKey.length() < 10) {
        robotSayUhOh();
        displayScrollingMessage("Set Gemini API Key in Web Dashboard Settings!", "GEMINI AI");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        robotSayUhOh();
        displayScrollingMessage("Connect to WiFi Router in Web Dashboard to use AI!", "NO INTERNET");
        return;
    }

    robotState = STATE_AI_THINKING;
    currentEmotion = EMOTION_CURIOUS_SCAN;
    robotSayCurious();

    // Show AI Thinking Screen
    if (oledReady) {
        display.clearDisplay();
        renderCyberHUD(millis() / 20);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 52);
        display.print(F("THINKING WITH AI..."));
        display.display();
    }

    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation for embedded lightweight client

    HTTPClient https;
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + geminiKey;

    if (https.begin(client, url)) {
        https.addHeader("Content-Type", "application/json");

        // Prepare System Prompt + User Query Payload
        String payload = "{\"contents\":[{\"parts\":[{\"text\":\""
                         "You are Desk Buddy, an intelligent, cute, sassy EMO robot companion. "
                         "Reply concisely in UNDER 15 WORDS. Always prepend ONE emotion tag: "
                         "[HAPPY], [LOVE], [ANGER], [CURIOUS], [COOL], [SLEEPY], [PARTY], [CAT], [MONEY], [HACKER], [KISS], or [SAD]. "
                         "User says: " + userPrompt + "\"}]}]}";

        int httpCode = https.POST(payload);
        if (httpCode == HTTP_CODE_OK || httpCode == 200) {
            String response = https.getString();
            Serial.println(F("[Gemini AI] Raw Response Received!"));

            // Simple fast JSON string parser for text content
            int textIdx = response.indexOf("\"text\": \"");
            if (textIdx != -1) {
                int start = textIdx + 9;
                int end = response.indexOf("\"", start);
                String aiText = response.substring(start, end);
                aiText.replace("\\n", " ");
                aiText.replace("\\\"", "\"");

                lastAIResponseText = aiText;
                Serial.print(F("[Gemini AI] Parsed Answer: "));
                Serial.println(aiText);

                // Parse emotion tag and trigger physical reaction
                if (aiText.indexOf("[LOVE]") != -1)          { currentEmotion = EMOTION_LOVE; robotSayLove(); }
                else if (aiText.indexOf("[PARTY]") != -1)    { currentEmotion = EMOTION_PARTY_DJ; robotSayParty(); }
                else if (aiText.indexOf("[ANGER]") != -1)    { currentEmotion = EMOTION_FIRE_RAGE; robotSayFire(); }
                else if (aiText.indexOf("[CAT]") != -1)      { currentEmotion = EMOTION_KAWAII_CAT; robotSayCat(); }
                else if (aiText.indexOf("[COOL]") != -1)     { currentEmotion = EMOTION_COOL_SUNGLASSES; targetServoAngle = SERVO_CENTER + 15.0f; }
                else if (aiText.indexOf("[MONEY]") != -1)    { currentEmotion = EMOTION_JACKPOT_MONEY; robotSayMoney(); }
                else if (aiText.indexOf("[HACKER]") != -1)   { currentEmotion = EMOTION_MATRIX_HACKER; robotSayHacker(); }
                else if (aiText.indexOf("[KISS]") != -1)     { currentEmotion = EMOTION_KAWAII_KISS; robotSayKiss(); }
                else if (aiText.indexOf("[SLEEPY]") != -1)   { currentEmotion = EMOTION_SLEEP; robotSaySleep(); }
                else if (aiText.indexOf("[SAD]") != -1)      { currentEmotion = EMOTION_RAINY_SAD; robotSaySad(); }
                else                                         { currentEmotion = EMOTION_TADA; robotSayTada(); }

                // Strip bracket tag from display text
                int closeBracket = aiText.indexOf("]");
                String cleanText = (closeBracket != -1) ? aiText.substring(closeBracket + 1) : aiText;
                cleanText.trim();

                displayScrollingMessage(cleanText, "🤖 DESK BUDDY AI");
            } else {
                robotSayUhOh();
                displayScrollingMessage("Could not parse AI reply!", "ERROR");
            }
        } else {
            Serial.print(F("[Gemini AI] HTTP Error: "));
            Serial.println(httpCode);
            robotSayUhOh();
            displayScrollingMessage("Gemini API Error (" + String(httpCode) + ")", "ERROR");
        }
        https.end();
    } else {
        robotSayUhOh();
        displayScrollingMessage("Connection to Gemini failed!", "ERROR");
    }

    robotState = STATE_AWAKE_IDLE;
    emotionResetTime = millis() + 4000;
}

// =========================================================================
// ☀️ INTERNET NTP CLOCK & WEATHER ENGINE
// =========================================================================

void updateInternetClockAndWeather() {
    if (WiFi.status() != WL_CONNECTED) return;

    unsigned long now = millis();
    if (now >= nextWeatherCheck) {
        nextWeatherCheck = now + 900000; // Update weather every 15 mins

        WiFiClient client;
        HTTPClient http;
        // Keyless free weather API (Open-Meteo)
        if (http.begin(client, "http://api.open-meteo.com/v1/forecast?latitude=23.8103&longitude=90.4125&current=temperature_2m,weather_code")) {
            int code = http.GET();
            if (code == 200) {
                String payload = http.getString();
                int tempIdx = payload.indexOf("\"temperature_2m\":");
                if (tempIdx != -1) {
                    float temp = payload.substring(tempIdx + 17, payload.indexOf(",", tempIdx)).toFloat();
                    currentTempC = (int)round(temp);
                    currentWeatherStr = (currentTempC > 28) ? "Warm & Sunny" : (currentTempC < 16) ? "Chilly" : "Pleasant";
                    Serial.print(F("[Weather] Temp: "));
                    Serial.print(currentTempC);
                    Serial.println(F(" C"));
                }
            }
            http.end();
        }
    }
}

// =========================================================================
// 🌐 FUTURISTIC GLASSMORPHIC WEB APP (4-TAB COMPANION DASHBOARD)
// =========================================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Desk Buddy AI EMO Companion</title>
<style>
:root{--bg:#070b14;--card:rgba(18,24,43,0.75);--primary:#00ffcc;--accent:#ff007f;--yellow:#fbbf24;--text:#f1f5f9;--glow:0 0 16px rgba(0,255,204,0.45)}
*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;-webkit-tap-highlight-color:transparent}
body{background:var(--bg);color:var(--text);padding:12px;background-image:radial-gradient(circle at 10% 20%,#1e1b4b,#070b14 80%);min-height:100vh}
.container{max-width:520px;margin:0 auto;padding-bottom:30px}
header{text-align:center;margin-bottom:12px}
h1{font-size:22px;color:var(--primary);text-shadow:var(--glow);letter-spacing:1px;display:flex;align-items:center;justify-content:center;gap:8px}
.pill{display:inline-block;padding:3px 10px;background:rgba(0,255,204,0.12);border:1px solid var(--primary);border-radius:20px;font-size:11px;margin-top:4px;color:var(--primary)}
.tabs{display:flex;background:rgba(0,0,0,0.35);border-radius:12px;padding:4px;margin-bottom:12px;gap:4px;border:1px solid rgba(255,255,255,0.06)}
.tab-btn{flex:1;background:transparent;border:none;color:#94a3b8;padding:8px 4px;font-size:12px;font-weight:600;border-radius:8px;cursor:pointer;transition:all 0.2s ease}
.tab-btn.active{background:rgba(0,255,204,0.15);color:var(--primary);box-shadow:var(--glow);border:1px solid rgba(0,255,204,0.3)}
.tab-content{display:none}
.tab-content.active{display:block}
.card{background:var(--card);backdrop-filter:blur(14px);border:1px solid rgba(255,255,255,0.08);border-radius:14px;padding:14px;margin-bottom:12px;box-shadow:0 8px 24px rgba(0,0,0,0.4)}
.card-title{font-size:13px;text-transform:uppercase;letter-spacing:1px;color:#94a3b8;margin-bottom:10px;display:flex;align-items:center;gap:6px}
.stat-row{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:8px}
.stat-box{background:rgba(0,0,0,0.3);padding:8px;border-radius:8px;text-align:center}
.stat-label{font-size:10px;color:#94a3b8}
.stat-val{font-size:15px;font-weight:bold;color:var(--primary)}
.btn-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
button.action-btn{background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.12);color:#fff;padding:10px 6px;border-radius:10px;font-size:12px;font-weight:600;cursor:pointer;display:flex;flex-direction:column;align-items:center;gap:4px;transition:all 0.15s ease}
button.action-btn:active{transform:scale(0.94);background:var(--primary);color:#000;border-color:var(--primary);box-shadow:var(--glow)}
.big-btn{grid-column:span 3;padding:12px;background:linear-gradient(135deg,#00ffcc,#0284c7);color:#000;font-size:14px;font-weight:bold;border:none}
.chat-box{background:rgba(0,0,0,0.35);border-radius:10px;padding:10px;min-height:140px;max-height:220px;overflow-y:auto;margin-bottom:10px;display:flex;flex-direction:column;gap:8px;font-size:13px}
.msg-bubble{padding:8px 12px;border-radius:10px;max-width:85%}
.msg-user{background:rgba(0,255,204,0.18);align-self:flex-end;border:1px solid rgba(0,255,204,0.3);color:#fff}
.msg-ai{background:rgba(255,0,127,0.18);align-self:flex-start;border:1px solid rgba(255,0,127,0.3);color:#fff}
.input-row{display:flex;gap:6px}
input[type=text],input[type=password],select{flex:1;background:rgba(0,0,0,0.4);border:1px solid rgba(255,255,255,0.15);color:#fff;padding:10px 12px;border-radius:10px;font-size:13px;outline:none}
input[type=text]:focus,input[type=password]:focus{border-color:var(--primary);box-shadow:var(--glow)}
.send-btn{background:var(--primary);color:#000;font-weight:bold;padding:10px 16px;border:none;border-radius:10px;cursor:pointer}
.mic-btn{background:rgba(255,0,127,0.2);border:1px solid var(--accent);color:#fff;padding:10px 12px;border-radius:10px;cursor:pointer}
.mic-btn.recording{background:var(--accent);box-shadow:0 0 16px rgba(255,0,127,0.7);animation:pulse 1s infinite}
@keyframes pulse{0%{transform:scale(1)}50%{transform:scale(1.08)}100%{transform:scale(1)}}
.chips{display:flex;gap:6px;overflow-x:auto;padding-bottom:6px;margin-top:8px}
.chip{background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.12);padding:4px 8px;border-radius:14px;font-size:11px;white-space:nowrap;cursor:pointer}
.slider-container{margin-top:6px}
input[type=range]{width:100%;accent-color:var(--primary);height:6px;border-radius:3px;background:rgba(255,255,255,0.1);outline:none}
.full-submit{width:100%;background:linear-gradient(135deg,#00ffcc,#0284c7);color:#000;font-weight:bold;padding:11px;border:none;border-radius:10px;cursor:pointer;margin-top:8px}
</style>
</head>
<body>
<div class="container">
<header>
<h1>🤖 DESK BUDDY AI</h1>
<div class="pill" id="conn-pill">● DUAL-MODE AP & STA ONLINE</div>
</header>

<div class="tabs">
<button class="tab-btn active" onclick="switchTab('tab-ai')">🧠 AI Brain</button>
<button class="tab-btn" onclick="switchTab('tab-ctrl')">🎮 Controls</button>
<button class="tab-btn" onclick="switchTab('tab-wifi')">📡 WiFi Router</button>
<button class="tab-btn" onclick="switchTab('tab-set')">⚙️ Settings</button>
</div>

<!-- TAB 1: GEMINI AI BRAIN -->
<div id="tab-ai" class="tab-content active">
<div class="card">
<div class="card-title">🧠 Google Gemini Voice & Text Brain</div>
<div class="chat-box" id="chat-log">
<div class="msg-bubble msg-ai">🤖 Hello! I am your Desk Buddy AI! Speak or type anything to me!</div>
</div>
<div class="input-row">
<button class="mic-btn" id="mic-btn" onclick="toggleVoiceInput()">🎙️</button>
<input type="text" id="ai-input" placeholder="Ask Desk Buddy anything..." onkeydown="if(event.key==='Enter')sendAI()">
<button class="send-btn" onclick="sendAI()">Ask</button>
</div>
<div class="chips">
<div class="chip" onclick="askChip('Tell me a funny joke!')">😂 Joke</div>
<div class="chip" onclick="askChip('How are you feeling today?')">❤️ Mood</div>
<div class="chip" onclick="askChip('What can you do?')">✨ Powers</div>
<div class="chip" onclick="askChip('Sing a happy robot song!')">🎶 Sing</div>
<div class="chip" onclick="askChip('Do a matrix hack trick!')">⚡ Hack</div>
</div>
</div>
</div>

<!-- TAB 2: ROBOT CONTROLS & GAMES -->
<div id="tab-ctrl" class="tab-content">
<div class="card">
<div class="card-title">❤️ Companion Vitals</div>
<div class="stat-row">
<div class="stat-box"><div class="stat-label">Affection</div><div class="stat-val" id="stat-aff">85%</div></div>
<div class="stat-box"><div class="stat-label">Energy</div><div class="stat-val" id="stat-eng">100%</div></div>
<div class="stat-box"><div class="stat-label">Hunger</div><div class="stat-val" id="stat-hng">90%</div></div>
</div>
</div>

<div class="card">
<div class="card-title">🕹️ Playable Mini-Games</div>
<div class="btn-grid">
<button class="action-btn big-btn" onclick="cmd('flap_start')">🎮 LAUNCH FLAPPY BIRD</button>
<button class="action-btn" onclick="cmd('flap_jump')">👆 Flap / Jump</button>
<button class="action-btn" onclick="cmd('rps')">✂️ Rock Paper Scissors</button>
<button class="action-btn" onclick="cmd('8ball')">🎲 Magic 8-Ball</button>
</div>
</div>

<div class="card">
<div class="card-title">🍕 Tamagotchi Pet Care</div>
<div class="btn-grid">
<button class="action-btn" onclick="cmd('pet')">🐱 Pet Head</button>
<button class="action-btn" onclick="cmd('snack_pizza')">🍕 Feed Pizza</button>
<button class="action-btn" onclick="cmd('snack_coffee')">☕ Give Coffee</button>
</div>
</div>

<div class="card">
<div class="card-title">🎭 Expression & Voice Matrix</div>
<div class="btn-grid">
<button class="action-btn" onclick="cmd('hello')">👋 Hello</button>
<button class="action-btn" onclick="cmd('love')">❤️ Love You</button>
<button class="action-btn" onclick="cmd('party')">🎉 Party DJ</button>
<button class="action-btn" onclick="cmd('shades')">🕶️ Sunglasses</button>
<button class="action-btn" onclick="cmd('pacman')">👾 Pacman</button>
<button class="action-btn" onclick="cmd('cat')">🐱 Neko Cat</button>
<button class="action-btn" onclick="cmd('fire')">🔥 Fire Rage</button>
<button class="action-btn" onclick="cmd('kiss')">💋 Kiss</button>
<button class="action-btn" onclick="cmd('matrix')">⚡ Matrix</button>
<button class="action-btn" onclick="cmd('sentry')">🚨 Sentry Mode</button>
<button class="action-btn" onclick="cmd('study')">👓 Study Mode</button>
<button class="action-btn" onclick="cmd('sleep')">🌙 Sleep</button>
</div>
</div>

<div class="card">
<div class="card-title">🦾 Physical Head Steering</div>
<div class="slider-container">
<input type="range" min="40" max="140" value="90" oninput="steer(this.value)">
</div>
</div>
</div>

<!-- TAB 3: WIFI ROUTER PROVISIONING -->
<div id="tab-wifi" class="tab-content">
<div class="card">
<div class="card-title">📡 Connect Desk Buddy to WiFi Router</div>
<p style="font-size:12px;color:#94a3b8;margin-bottom:10px">Connect to any local 2.4GHz WiFi network to give Desk Buddy full Internet & Gemini AI access anywhere!</p>
<button class="full-submit" style="margin-bottom:12px" onclick="scanWifi()">🔍 Scan Nearby 2.4GHz Networks</button>
<div style="margin-bottom:8px">
<label style="font-size:11px;color:#94a3b8">SELECT NETWORK (SSID):</label>
<select id="wifi-ssid-select" style="width:100%;margin-top:4px">
<option value="">-- Click Scan to find networks --</option>
</select>
</div>
<div style="margin-bottom:8px">
<label style="font-size:11px;color:#94a3b8">WIFI PASSWORD:</label>
<input type="password" id="wifi-pass" placeholder="Enter WiFi Password" style="width:100%;margin-top:4px">
</div>
<button class="full-submit" onclick="saveWifi()">💾 Connect & Save to Flash</button>
</div>
</div>

<!-- TAB 4: SETTINGS & BILLBOARD -->
<div id="tab-set" class="tab-content">
<div class="card">
<div class="card-title">🔑 Google Gemini API Key</div>
<input type="password" id="gemini-key-input" placeholder="AIzaSy..." style="width:100%;margin-bottom:8px">
<button class="full-submit" onclick="saveGeminiKey()">💾 Save Gemini Key</button>
</div>

<div class="card">
<div class="card-title">📢 OLED Billboard Broadcaster</div>
<input type="text" id="billboard-msg" placeholder="Type custom message to scroll on OLED..." style="width:100%;margin-bottom:8px">
<button class="full-submit" onclick="sendBillboard()">🚀 Send to OLED Screen</button>
</div>
</div>

</div>

<script>
function switchTab(tabId){
  document.querySelectorAll('.tab-btn').forEach(b=>b.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(c=>c.classList.remove('active'));
  event.target.classList.add('active');
  document.getElementById(tabId).classList.add('active');
}
function cmd(action){fetch('/api?cmd='+action);}
function steer(angle){fetch('/api?steer='+angle);}

function sendAI(){
  const inp=document.getElementById('ai-input');
  const q=inp.value.trim();
  if(!q)return;
  addChat(q,'user');
  inp.value='';
  addChat('🤖 Thinking with Gemini...','ai');
  fetch('/api/gemini/ask',{method:'POST',body:q})
  .then(r=>r.text())
  .then(ans=>{
    const c=document.getElementById('chat-log');
    c.lastChild.textContent='🤖 '+ans;
    c.scrollTop=c.scrollHeight;
  });
}

function askChip(q){
  document.getElementById('ai-input').value=q;
  sendAI();
}

function addChat(text,type){
  const c=document.getElementById('chat-log');
  const b=document.createElement('div');
  b.className='msg-bubble msg-'+type;
  b.textContent=(type==='user'?'👤 ':'')+text;
  c.appendChild(b);
  c.scrollTop=c.scrollHeight;
}

// Web Speech API Voice Recognition
let recognition=null;
function toggleVoiceInput(){
  if(!('webkitSpeechRecognition' in window)&&!('SpeechRecognition' in window)){
    alert('Voice speech recognition not supported on this browser. Use Chrome, Safari, or Edge.');
    return;
  }
  const btn=document.getElementById('mic-btn');
  if(!recognition){
    const SpeechRec=window.SpeechRecognition||window.webkitSpeechRecognition;
    recognition=new SpeechRec();
    recognition.continuous=false;
    recognition.interimResults=false;
    recognition.onstart=()=>{btn.classList.add('recording');};
    recognition.onresult=(e)=>{
      const transcript=e.results[0][0].transcript;
      document.getElementById('ai-input').value=transcript;
      btn.classList.remove('recording');
      sendAI();
    };
    recognition.onerror=()=>{btn.classList.remove('recording');};
    recognition.onend=()=>{btn.classList.remove('recording');};
  }
  recognition.start();
}

function scanWifi(){
  const sel=document.getElementById('wifi-ssid-select');
  sel.innerHTML='<option>Scanning 2.4GHz WiFi networks...</option>';
  fetch('/api/wifi/scan',{method:'POST'})
  .then(r=>r.json())
  .then(nets=>{
    sel.innerHTML='';
    nets.forEach(n=>{
      const opt=document.createElement('option');
      opt.value=n.ssid;
      opt.textContent=n.ssid+' ('+n.rssi+' dBm)';
      sel.appendChild(opt);
    });
  });
}

function saveWifi(){
  const s=document.getElementById('wifi-ssid-select').value;
  const p=document.getElementById('wifi-pass').value;
  fetch('/api/wifi/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:s,pass:p})})
  .then(r=>r.text())
  .then(m=>alert('WiFi saved! Desk Buddy is connecting to '+s));
}

function saveGeminiKey(){
  const k=document.getElementById('gemini-key-input').value.trim();
  fetch('/api/gemini/key',{method:'POST',body:k})
  .then(r=>r.text())
  .then(m=>alert('Gemini API Key Saved to Flash!'));
}

function sendBillboard(){
  const msg=document.getElementById('billboard-msg').value.trim();
  if(!msg)return;
  fetch('/api/billboard',{method:'POST',body:msg});
}
</script>
</body>
</html>
)rawliteral";

// =========================================================================
// 🌐 WEB SERVER REQUEST HANDLERS
// =========================================================================

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleCommand() {
    lastInteractionTime = millis();
    affectionLevel = min(100, affectionLevel + 3);

    if (server.hasArg("steer")) {
        int a = server.arg("steer").toInt();
        targetServoAngle = constrain(a, 40, 140);
        server.send(200, "text/plain", "OK");
        return;
    }

    if (server.hasArg("cmd")) {
        String c = server.arg("cmd");
        if (c == "hello")        { robotSayHello(); }
        else if (c == "love")    { currentEmotion = EMOTION_LOVE; robotSayLove(); emotionResetTime = millis() + 4000; }
        else if (c == "party")   { currentEmotion = EMOTION_PARTY_DJ; robotSayParty(); emotionResetTime = millis() + 5000; }
        else if (c == "shades")  { currentEmotion = EMOTION_COOL_SUNGLASSES; targetServoAngle = SERVO_CENTER + 15.0f; emotionResetTime = millis() + 4500; }
        else if (c == "pacman")  { currentEmotion = EMOTION_GAMER_PACMAN; robotSayGame(); emotionResetTime = millis() + 4500; }
        else if (c == "cat" || c == "pet") { currentEmotion = EMOTION_KAWAII_CAT; robotSayCat(); emotionResetTime = millis() + 4500; }
        else if (c == "fire")    { currentEmotion = EMOTION_FIRE_RAGE; robotSayFire(); emotionResetTime = millis() + 4500; }
        else if (c == "kiss")    { currentEmotion = EMOTION_KAWAII_KISS; robotSayKiss(); emotionResetTime = millis() + 4500; }
        else if (c == "matrix")  { currentEmotion = EMOTION_MATRIX_HACKER; robotSayHacker(); emotionResetTime = millis() + 4500; }
        else if (c == "sentry")  { robotState = STATE_SENTRY_GUARD; sentryActive = true; robotSaySentry(); }
        else if (c == "study")   { robotState = STATE_FOCUS_STUDY; currentEmotion = EMOTION_FOCUS_STUDY; robotSayFocus(); emotionResetTime = millis() + 15000; }
        else if (c == "sleep")   { robotState = STATE_DEEP_SLEEP; currentEmotion = EMOTION_SLEEP; robotSaySleep(); }
        else if (c == "flap_start") { startFlappyGame(); }
        else if (c == "flap_jump")  { jumpFlappyBird(); }
        else if (c == "rps")        { startRockPaperScissors(); }
        else if (c == "8ball") {
            currentEmotion = EMOTION_MAGIC_8BALL;
            const char* answers[] = {"YES! 100%", "NO WAY!", "SIGNS SAY YES", "TRY AGAIN", "ABSOLUTELY!"};
            magic8Answer = answers[random(0, 5)];
            robotSayCurious();
            emotionResetTime = millis() + 4500;
        }
        else if (c.startsWith("snack")) { feedSnack("Pizza"); }
    }
    server.send(200, "text/plain", "OK");
}

void handleStatus() {
    String json = "{";
    json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"sta_ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Disconnected") + "\",";
    json += "\"sta_ssid\":\"" + staSSID + "\",";
    json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"affection\":" + String(affectionLevel) + ",";
    json += "\"energy\":" + String(energyLevel) + ",";
    json += "\"hunger\":" + String(hungerLevel) + ",";
    json += "\"servo_angle\":" + String((int)currentServoAngle) + ",";
    json += "\"temp_c\":" + String(currentTempC) + ",";
    json += "\"weather\":\"" + currentWeatherStr + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void handleWifiScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleWifiSave() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        int sIdx = body.indexOf("\"ssid\":\"");
        int pIdx = body.indexOf("\"pass\":\"");
        if (sIdx != -1) {
            int sEnd = body.indexOf("\"", sIdx + 8);
            staSSID = body.substring(sIdx + 8, sEnd);
            int pEnd = body.indexOf("\"", pIdx + 8);
            staPass = body.substring(pIdx + 8, pEnd);

            prefs.begin(NVS_NAMESPACE, false);
            prefs.putString("sta_ssid", staSSID);
            prefs.putString("sta_pass", staPass);
            prefs.end();

            Serial.print(F("[WiFi] Saved Router SSID to Flash: "));
            Serial.println(staSSID);

            WiFi.begin(staSSID.c_str(), staPass.c_str());
            server.send(200, "text/plain", "SAVED");
            return;
        }
    }
    server.send(400, "text/plain", "BAD REQUEST");
}

void handleGeminiKey() {
    if (server.hasArg("plain")) {
        geminiKey = server.arg("plain");
        geminiKey.trim();

        prefs.begin(NVS_NAMESPACE, false);
        prefs.putString("gemini_key", geminiKey);
        prefs.end();

        Serial.println(F("[Gemini] API Key Saved to Flash!"));
        server.send(200, "text/plain", "SAVED");
        return;
    }
    server.send(400, "text/plain", "BAD REQUEST");
}

void handleGeminiAsk() {
    if (server.hasArg("plain")) {
        String prompt = server.arg("plain");
        prompt.trim();
        askGeminiAI(prompt);
        server.send(200, "text/plain", lastAIResponseText);
        return;
    }
    server.send(400, "text/plain", "BAD REQUEST");
}

void handleBillboard() {
    if (server.hasArg("plain")) {
        String msg = server.arg("plain");
        msg.trim();
        robotSayTada();
        displayScrollingMessage(msg, "📢 BROADCAST");
        server.send(200, "text/plain", "DISPLAYED");
        return;
    }
    server.send(400, "text/plain", "BAD REQUEST");
}

// =========================================================================
// SETUP
// =========================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("========================================================="));
    Serial.println(F(" 🤖 ESP32 DESK BUDDY: EMO LIVING AI COMPANION MASTER     "));
    Serial.println(F("========================================================="));

    // 1. Audio DAC on GPIO 25
    pinMode(AUDIO_DAC_PIN, OUTPUT);
    dacWrite(AUDIO_DAC_PIN, 0);

    // 2. Microphone Sensor on GPIO 19
#if ENABLE_SOUND_SENSOR
    pinMode(MIC_DO_PIN, INPUT_PULLUP);
    Serial.println(F("[Hardware] Microphone Sound Sensor Initialized (GPIO 19)"));
#endif

    // 3. SG90 Servo on GPIO 18
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    headServo.setPeriodHertz(50);
    headServo.attach(SERVO_PIN, 500, 2400);
    headServo.write((int)SERVO_CENTER);

    // 4. I2C OLED on GPIO 21 / 22
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);

    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C) || display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
        oledReady = true;
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);

        display.setTextSize(2);
        display.setCursor(14, 10);
        display.println(F("DESK BUDDY"));
        display.setTextSize(1);
        display.setCursor(14, 34);
        display.println(F("STARTING DUAL WIFI..."));
        display.setCursor(14, 46);
        display.println(F("AP: DeskBuddy-WiFi"));
        display.drawRoundRect(2, 2, 124, 60, 6, SSD1306_WHITE);
        display.display();
    }

    // 5. Load Saved NVS Settings
    prefs.begin(NVS_NAMESPACE, true);
    staSSID = prefs.getString("sta_ssid", "");
    staPass = prefs.getString("sta_pass", "");
    geminiKey = prefs.getString("gemini_key", "");
    gmtOffsetHours = prefs.getInt("gmt_offset", 0);
    prefs.end();

    // 6. Start Simultaneous Dual-Mode WiFi (AP + STA)
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_DEFAULT_SSID, AP_DEFAULT_PASS);
    IPAddress apIP = WiFi.softAPIP();

    Serial.print(F("[WiFi AP] Direct Hotspot Active: "));
    Serial.println(AP_DEFAULT_SSID);
    Serial.print(F("[WiFi AP] Direct Dashboard URL: http://"));
    Serial.println(apIP);

    if (staSSID.length() > 0) {
        Serial.print(F("[WiFi STA] Auto-connecting to saved Router: "));
        Serial.println(staSSID);
        WiFi.begin(staSSID.c_str(), staPass.c_str());
    }

    // 7. Start mDNS (http://deskbuddy.local)
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.println(F("[mDNS] Companion accessible at: http://deskbuddy.local"));
    }

    // 8. Register Web Server Endpoints
    server.on("/", handleRoot);
    server.on("/api", handleCommand);
    server.on("/api/status", handleStatus);
    server.on("/api/wifi/scan", HTTP_POST, handleWifiScan);
    server.on("/api/wifi/save", HTTP_POST, handleWifiSave);
    server.on("/api/gemini/key", HTTP_POST, handleGeminiKey);
    server.on("/api/gemini/ask", HTTP_POST, handleGeminiAsk);
    server.on("/api/billboard", HTTP_POST, handleBillboard);
    server.begin();
    Serial.println(F("[Web] HTTP Glassmorphic Dashboard Online!"));

    delay(800);
    robotSayTada(); // Startup cheer
    delay(500);

    lastInteractionTime = millis();
    nextSpontaneousBehavior = millis() + random(14000, 24000);
}

// =========================================================================
// MAIN COMPANION LIFE & GAME LOOP
// =========================================================================
void loop() {
    unsigned long now = millis();

    // 1. Process HTTP Web Server Requests
    server.handleClient();

    // 2. Microphone Sound & Clap Detection Engine
    checkMicrophoneClap();

    // 3. Hardware Capacitive Touch Petting Detection (GPIO 4)
#if ENABLE_TOUCH_PIN
    int touchVal = touchRead(TOUCH_HEAD_PIN);
    if (touchVal < 38) {
        if (!isBeingPetted) {
            isBeingPetted = true;
            lastInteractionTime = now;
            if (robotState == STATE_GAME_FLAPPY) {
                jumpFlappyBird();
            } else {
                affectionLevel = min(100, affectionLevel + 8);
                robotState = STATE_HAPPY_AFFECTION;
                currentEmotion = EMOTION_KAWAII_CAT;
                robotSayCat();
                emotionResetTime = now + 4000;
            }
        }
    } else {
        isBeingPetted = false;
    }
#endif

    // 4. Serial Keyboard Controls & Games
    if (Serial.available() > 0) {
        char ch = Serial.read();
        if (ch != '\r' && ch != '\n' && ch != ' ') {
            lastInteractionTime = now;

            if (robotState == STATE_GAME_FLAPPY) {
                jumpFlappyBird();
            } else if (robotState == STATE_DEEP_SLEEP || robotState == STATE_DROWSY_NAP) {
                robotState = STATE_AWAKE_IDLE;
                robotSayHello();
            } else {
                switch (ch) {
                    case '1': case 'h': case 'H': robotSayHello(); break;
                    case '2': case 'l': case 'L': currentEmotion = EMOTION_LOVE; robotSayLove(); emotionResetTime = now + 4000; break;
                    case '3': case 't': case 'T': currentEmotion = EMOTION_TADA; robotSayTada(); emotionResetTime = now + 4000; break;
                    case '4': case 'p': case 'P': case 'd': case 'D': currentEmotion = EMOTION_PARTY_DJ; robotSayParty(); emotionResetTime = now + 5000; break;
                    case '5': currentEmotion = EMOTION_CURIOUS_SCAN; robotSayCurious(); emotionResetTime = now + 4000; break;
                    case '6': case 'u': case 'U': currentEmotion = EMOTION_UHOH_ALERT; robotSayUhOh(); emotionResetTime = now + 3500; break;
                    case '7': robotState = STATE_DEEP_SLEEP; currentEmotion = EMOTION_SLEEP; robotSaySleep(); break;
                    case 'g': case 'G': currentEmotion = EMOTION_COOL_SUNGLASSES; targetServoAngle = SERVO_CENTER + 15.0f; emotionResetTime = now + 4500; break;
                    case 'x': case 'X': startFlappyGame(); break;
                    case 'r': case 'R': startRockPaperScissors(); break;
                    case 'e': case 'E': feedSnack("Pizza"); break;
                    case 'a': case 'A': robotState = STATE_SENTRY_GUARD; sentryActive = true; robotSaySentry(); break;
                    case 'b': case 'c': case 'C': currentEmotion = EMOTION_KAWAII_CAT; robotSayCat(); emotionResetTime = now + 4500; break;
                    case 'f': case 'F': currentEmotion = EMOTION_FIRE_RAGE; robotSayFire(); emotionResetTime = now + 4500; break;
                    case 'z': case 'Z': currentEmotion = EMOTION_HYPNO_DIZZY; robotSayDizzy(); emotionResetTime = now + 4000; break;
                    case 'm': case 'M': currentEmotion = EMOTION_JACKPOT_MONEY; robotSayMoney(); emotionResetTime = now + 4500; break;
                    case 'k': case 'K': currentEmotion = EMOTION_MATRIX_HACKER; robotSayHacker(); emotionResetTime = now + 4500; break;
                    case 'v': case 'V': currentEmotion = EMOTION_KAWAII_KISS; robotSayKiss(); emotionResetTime = now + 4500; break;
                    case 'o': case 'O': robotState = STATE_FOCUS_STUDY; currentEmotion = EMOTION_FOCUS_STUDY; robotSayFocus(); emotionResetTime = now + 15000; break;
                    case '8': {
                        currentEmotion = EMOTION_MAGIC_8BALL;
                        const char* answers[] = {"YES! 100%", "NO WAY!", "SIGNS SAY YES", "TRY AGAIN", "ABSOLUTELY!"};
                        magic8Answer = answers[random(0, 5)];
                        robotSayCurious();
                        emotionResetTime = now + 4500;
                        break;
                    }
                    case 'i': case 'I':
                        robotState = STATE_AWAKE_IDLE;
                        currentEmotion = EMOTION_IDLE;
                        targetServoAngle = SERVO_CENTER;
                        break;
                }
            }
        }
    }

    // 5. Game Modes & Sentry Engine
    if (robotState == STATE_GAME_FLAPPY) {
        updateFlappyGame();
        updateServoMotion();
        return;
    }

    if (robotState == STATE_SENTRY_GUARD) {
        if (now >= nextSentryPan) {
            nextSentryPan = now + 120;
            if (sentryPanDir) {
                sentryPanAngle += 3.0f;
                if (sentryPanAngle >= 135.0f) sentryPanDir = false;
            } else {
                sentryPanAngle -= 3.0f;
                if (sentryPanAngle <= 45.0f) sentryPanDir = true;
            }
            targetServoAngle = sentryPanAngle;
        }
        renderSentryAlert(millis() / 100);
        display.display();
        updateServoMotion();
        return;
    }

    // 6. Living Auto-Nap after 90s idle
    if (robotState == STATE_AWAKE_IDLE && (now - lastInteractionTime > 90000)) {
        robotState = STATE_DEEP_SLEEP;
        currentEmotion = EMOTION_SLEEP;
        robotSaySleep();
    }

    // 7. Spontaneous Daydreams
    if (robotState == STATE_AWAKE_IDLE && now >= nextSpontaneousBehavior) {
        nextSpontaneousBehavior = now + random(16000, 30000);
        int act = random(0, 7);
        switch (act) {
            case 0: currentEmotion = EMOTION_KAWAII_CAT; robotSayCat(); emotionResetTime = now + 4000; break;
            case 1: currentEmotion = EMOTION_GAMER_PACMAN; robotSayGame(); emotionResetTime = now + 4000; break;
            case 2: currentEmotion = EMOTION_KAWAII_KISS; robotSayKiss(); emotionResetTime = now + 3500; break;
            case 3: currentEmotion = EMOTION_COOL_SUNGLASSES; targetServoAngle = SERVO_CENTER + 15.0f; emotionResetTime = now + 4000; break;
            case 4: currentEmotion = EMOTION_MATRIX_HACKER; robotSayHacker(); emotionResetTime = now + 4000; break;
            case 5: currentEmotion = EMOTION_LOVE; robotSayLove(); emotionResetTime = now + 3500; break;
            case 6: currentEmotion = EMOTION_TADA; robotSayTada(); emotionResetTime = now + 3500; break;
        }
    }

    if (currentEmotion != EMOTION_IDLE && robotState == STATE_AWAKE_IDLE && now >= emotionResetTime) {
        currentEmotion = EMOTION_IDLE;
        targetServoAngle = SERVO_CENTER;
    }

    // 8. Biological Gaze & Blinking
    if (robotState == STATE_AWAKE_IDLE && currentEmotion == EMOTION_IDLE) {
        if (now >= nextGazeTime) {
            int r = random(0, 5);
            if (r == 0)      { targetLookX = -0.85f; targetLookY = -0.2f; }
            else if (r == 1) { targetLookX = 0.85f;  targetLookY = 0.2f; }
            else if (r == 2) { targetLookX = 0.0f;   targetLookY = -0.4f; }
            else             { targetLookX = 0.0f;   targetLookY = 0.0f; }

            targetServoAngle = SERVO_CENTER + (targetLookX * 28.0f);
            nextGazeTime = now + random(2000, 5000);
        }

        if (!isBlinking && now >= nextBlinkTime) {
            isBlinking = true;
            blinkProgress = 0.0f;
            doubleBlinkPending = (random(0, 3) == 0);
            nextBlinkTime = now + random(2500, 5500);
        }

        lookX += (targetLookX - lookX) * 0.22f;
        lookY += (targetLookY - lookY) * 0.22f;

        if (isBlinking) {
            blinkProgress += 0.35f;
            if (blinkProgress >= 1.0f) {
                isBlinking = false;
                if (doubleBlinkPending) {
                    doubleBlinkPending = false;
                    nextBlinkTime = now + 150;
                }
            }
        }
    }

    // 9. Periodic Internet Weather Check
    updateInternetClockAndWeather();

    // 10. Render Screen
    if (oledReady) {
        float openRatio = isBlinking ? (1.0f - blinkProgress) : 1.0f;

        if (robotState == STATE_DEEP_SLEEP) {
            renderSleepMode(millis() / 120);
        } else if (robotState == STATE_SNACK_FEEDING) {
            renderSnackEat(millis() / 150);
        } else if (robotState == STATE_GAME_RPS) {
            renderRPS(rpsRobotChoice);
        } else {
            switch (currentEmotion) {
                case EMOTION_GAMER_PACMAN: renderGamerPacman(millis() / 150); break;
                case EMOTION_RAINY_SAD: renderRainySad(millis() / 80); break;
                case EMOTION_FIRE_RAGE: renderFireRage(millis() / 40); break;
                case EMOTION_HYPNO_DIZZY: renderHypnoDizzy(millis() / 30); break;
                case EMOTION_KAWAII_CAT: renderKawaiiCat(millis() / 200); break;
                case EMOTION_JACKPOT_MONEY: renderJackpotMoney(millis() / 60); break;
                case EMOTION_MATRIX_HACKER: renderMatrixHacker(millis() / 40); break;
                case EMOTION_KAWAII_KISS: renderKawaiiKiss(millis() / 60); break;
                case EMOTION_FOCUS_STUDY: renderFocusStudy(millis() / 100); break;
                case EMOTION_MAGIC_8BALL: renderMagic8BallScreen(magic8Answer); break;
                case EMOTION_COOL_SUNGLASSES: renderEmoCoolSunglasses(millis() / 20); break;
                case EMOTION_LOVE: renderWalleLoveFace((millis() / 80) % 6); break;
                case EMOTION_PARTY_DJ: renderEmoPartyDJ(millis() / 30); break;
                case EMOTION_CURIOUS_SCAN: renderCyberHUD(millis() / 25); break;
                case EMOTION_SLEEP: renderSleepMode(millis() / 120); break;
                case EMOTION_IDLE:
                default:
                    renderLivingIdleFace(lookX, lookY, openRatio);
                    break;
            }
        }
        display.display();
    }

    // 11. Update Servo Motion
    updateServoMotion();

    delay(20);
}
