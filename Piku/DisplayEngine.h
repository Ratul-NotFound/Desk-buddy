#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <math.h>
#include "config.h"
#include "AudioEngine.h"

enum RobotEmotion {
    EMOTION_IDLE, EMOTION_HELLO, EMOTION_LOVE, EMOTION_TADA, EMOTION_PARTY_DJ,
    EMOTION_CURIOUS_SCAN, EMOTION_UHOH_ALERT, EMOTION_COOL_SUNGLASSES,
    EMOTION_SLEEP, EMOTION_GAMER_PACMAN, EMOTION_RAINY_SAD, EMOTION_FIRE_RAGE,
    EMOTION_HYPNO_DIZZY, EMOTION_KAWAII_CAT, EMOTION_JACKPOT_MONEY,
    EMOTION_MATRIX_HACKER, EMOTION_KAWAII_KISS, EMOTION_FOCUS_STUDY,
    EMOTION_MAGIC_8BALL, EMOTION_CLOCK_DISPLAY, EMOTION_WEATHER_DISPLAY,
    EMOTION_SENTRY_ALERT, EMOTION_SNACK_EAT, EMOTION_RPS_SHOW
};

struct EyeShape {
    float wL, hL, rL;   // left eye width/height/radius
    float wR, hR, rR;   // right eye width/height/radius
    float pupilSize;     // 0 = no pupils
    float offsetY;       // vertical shift
    bool  winkLeft;
};

class DisplayEngine {
public:
    void init();
    void morphToEmotion(RobotEmotion e, int durationMs = 300);
    void setGaze(float gx, float gy) { _gazeX = gx; _gazeY = gy; }
    void triggerBlink();
    void update(AudioEngine* audio);
    void startScrollMessage(const String& text, const char* title = nullptr);
    void showVolumeHUD(int vol, bool muted);
    bool isScrolling() const { return _scrolling; }

    void setClockWeather(const String& timeStr, const String& dateStr, int tempC, int hum, const String& cond);
    void setRPSChoice(const String& choice)     { _rpsChoice = choice; }
    void setMagic8Answer(const String& answer)   { _magic8Answer = answer; }

    // For mini-games / special screens — called directly
    void renderFlappyGame(int birdY, float vel, int score, int hi, int pipeX, int pipeGapY, bool over);
    void renderRPS(const char* choice);
    void renderSentryAlert(int tick);
    void renderSnackEat(int frame);
    void renderMagic8Ball(const char* answer);
    void renderClockScreen(const char* timeStr, const char* dateStr);
    void renderWeatherScreen(int tempC, int humidity, const char* condition);

private:
    Adafruit_SSD1306 _disp;
    bool _ready = false;

    // Eye animation state
    EyeShape _current;
    EyeShape _from;
    EyeShape _to;
    unsigned long _morphStart = 0;
    int           _morphDuration = 300;

    float _gazeX = 0, _gazeY = 0;

    // Blink
    bool  _blinking = false;
    float _blinkProg = 0;
    unsigned long _nextBlink = 0;

    // Scroll message
    bool   _scrolling   = false;
    String _scrollText;
    String _scrollTitle;
    int    _scrollX     = 128;
    unsigned long _nextScrollTick = 0;

    RobotEmotion _currentEmotion = EMOTION_IDLE;

    // Overlay & animation tick counter
    unsigned long _overlayFrame = 0;

    // Live clock and weather data for overlays
    String _timeStr     = "--:--";
    String _dateStr     = "---";
    int    _tempC       = 25;
    int    _humidity    = 65;
    String _weatherCond = "Sunny";

    // Dynamic mini-game states
    String _rpsChoice    = "ROCK";
    String _magic8Answer = "YES!";

    void   _drawEyes(const EyeShape& s, float gx, float gy, float openRatio);
    void   _drawOverlay(RobotEmotion e);
    void   _drawMouth(int shape);
    void   _drawHeart(int cx, int cy, int size);
    EyeShape _targetShapeFor(RobotEmotion e);
    void   _renderSpeechFace(int mouthShape, const char* subtitle);
    void   _renderIdle(float gx, float gy, float openRatio);
    void   _renderSpecial();
    void   _updateScroll();
    float  _lerp(float a, float b, float t) { return a + (b - a) * t; }
    EyeShape _lerpShape(const EyeShape& a, const EyeShape& b, float t);
};
