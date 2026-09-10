# PIKU 2.0 — Full Autonomous Desk Companion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild PIKU from a 2,804-line blocking monolith into a modular, FreeRTOS dual-core autonomous AI desk companion with persistent owner memory, smooth OLED animation, non-blocking audio, and a lightweight web UI.

**Architecture:** Eight C++ modules (ServoEngine, AudioEngine, SensorEngine, DisplayEngine, SoulEngine, BrainEngine, NetworkEngine, WebUI) loaded from `Piku.ino` which launches two FreeRTOS tasks — Core 0 for network/AI and Core 1 for the companion soul loop. The existing `voice_samples.h`, `config.h`, and `secrets.h` are preserved unchanged.

**Tech Stack:** ESP32 Arduino framework, FreeRTOS (built-in), Adafruit SSD1306 ^2.5.7, Adafruit GFX ^1.11.5, ESP32Servo ^1.1.0, Preferences (NVS), WiFi, WebServer, HTTPClient, WiFiClientSecure, ESPmDNS, time.h

**Spec:** `docs/superpowers/specs/2026-09-10-piku-v2-design.md`

## Global Constraints

- ESP32 DevKit V1, Arduino framework, serial 115200
- Partition: "Huge APP (3MB No OTA/1MB SPIFFS)" — add `board_build.partitions = huge_app.csv` in platformio.ini
- `voice_samples.h` arrays: PROGMEM, never copied to RAM
- Audio sample rate: 16,000 Hz (`VOICE_SAMPLE_RATE 16000`)
- OLED: SSD1306 at I2C address 0x3C, GPIO 21/22, fast mode 400kHz
- Servo: GPIO 18, limits 40-140°, center 90°, easing factor 0.15f
- Gemini response: max 18 words, must start with `[EMOTION:intensity]` tag
- Web UI: vanilla HTML/CSS/JS only — no CDN, no external dependencies, page < 12 KB
- Language: English only
- NVS namespace: "piku"
- All modules communicate through shared global structs declared in `Piku.ino` and passed by pointer

---

## Task 1: Project Scaffold + ServoEngine

**Files:**
- Modify: `Piku/platformio.ini`
- Create: `Piku/ServoEngine.h`
- Create: `Piku/ServoEngine.cpp`

**Interfaces:**
- Produces: `ServoEngine::init()`, `ServoEngine::setTarget(float angle)`, `ServoEngine::update()`, `ServoEngine::triggerWiggle()`, `ServoEngine::performGesture(GestureType g)`
- GestureType enum: `GESTURE_NOD, GESTURE_SHAKE, GESTURE_TILT_LEFT, GESTURE_TILT_RIGHT, GESTURE_CURIOUS, GESTURE_WIGGLE`

- [ ] **Step 1: Update platformio.ini**

```ini
[platformio]
src_dir = Piku

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.partitions = huge_app.csv

lib_deps =
    adafruit/Adafruit SSD1306 @ ^2.5.7
    adafruit/Adafruit GFX Library @ ^1.11.5
    madhephaestus/ESP32Servo @ ^1.1.0
```

- [ ] **Step 2: Create ServoEngine.h**

```cpp
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
```

- [ ] **Step 3: Create ServoEngine.cpp**

```cpp
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
```

- [ ] **Step 4: Verify compiles (no hardware needed yet)**

Build with: `pio run -e esp32dev` (or Arduino IDE Verify). Expected: compiles clean. If ESP32Servo not found, run `pio lib install "ESP32Servo"`.

- [ ] **Step 5: Commit**

```bash
git add Piku/ServoEngine.h Piku/ServoEngine.cpp platformio.ini
git commit -m "feat(piku2): scaffold + ServoEngine module with FreeRTOS-safe motion"
```

---

## Task 2: AudioEngine — Non-Blocking FreeRTOS DAC Queue

**Files:**
- Create: `Piku/AudioEngine.h`
- Create: `Piku/AudioEngine.cpp`

**Interfaces:**
- Produces: `AudioEngine::init()`, `AudioEngine::playHD(const uint8_t* data, int len, int mouthShape, const char* subtitle)`, `AudioEngine::playChirp(int startFreq, int endFreq, int durationMs)`, `AudioEngine::playPhonemes(int wordCount, const char* subtitle)`, `AudioEngine::setVolume(int vol)`, `AudioEngine::isMuted()`, `AudioEngine::currentMouthShape` (volatile int), `AudioEngine::micMuteUntil` (volatile unsigned long), `AudioEngine::startTask()` (launches FreeRTOS task on Core 1)

- [ ] **Step 1: Create AudioEngine.h**

```cpp
#pragma once
#include <Arduino.h>
#include "config.h"

struct AudioJob {
    enum JobType : uint8_t { JOB_HD_SAMPLE, JOB_CHIRP, JOB_PHONEME } type;
    const uint8_t* data;
    int dataLen;
    int startFreq;
    int endFreq;
    int durationMs;
    int wordCount;
    char subtitle[64];
    int mouthShape;
};

class AudioEngine {
public:
    void init(int initialVolume);
    void startTask();            // launch FreeRTOS task — call once after init()
    void playHD(const uint8_t* data, int len, int mouthShape = -1, const char* subtitle = nullptr);
    void playChirp(int startFreq, int endFreq, int durationMs);
    void playPhonemes(int wordCount, const char* subtitle = nullptr);
    void setVolume(int vol);
    int  getVolume() const { return _volume; }
    bool isMuted()   const { return _muted; }
    void setMuted(bool m)  { _muted = m; }

    volatile int           currentMouthShape = -1;
    volatile unsigned long micMuteUntil      = 0;

private:
    QueueHandle_t _queue;
    int  _volume = 80;
    bool _muted  = false;

    void _playHDInternal(const uint8_t* data, int len, int vol);
    void _playChirpInternal(int sf, int ef, int dur, int vol);
    void _playPhonemesInternal(int words, const char* sub, int vol);

    static void _audioTask(void* param);
};
```

- [ ] **Step 2: Create AudioEngine.cpp**

```cpp
#include "AudioEngine.h"
#include "voice_samples.h"

void AudioEngine::init(int initialVolume) {
    _volume = constrain(initialVolume, 0, 100);
    pinMode(AUDIO_DAC_PIN, OUTPUT);
    dacWrite(AUDIO_DAC_PIN, 0);
    _queue = xQueueCreate(4, sizeof(AudioJob));
}

void AudioEngine::startTask() {
    xTaskCreatePinnedToCore(_audioTask, "AudioTask", 4096, this, 1, nullptr, 1);
}

void AudioEngine::setVolume(int vol) {
    _volume = constrain(vol, 0, 100);
    _muted  = (_volume == 0);
}

void AudioEngine::playHD(const uint8_t* data, int len, int mouthShape, const char* subtitle) {
    AudioJob job;
    job.type      = AudioJob::JOB_HD_SAMPLE;
    job.data      = data;
    job.dataLen   = len;
    job.mouthShape = mouthShape;
    if (subtitle) strncpy(job.subtitle, subtitle, 63);
    else job.subtitle[0] = '\0';
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::playChirp(int startFreq, int endFreq, int durationMs) {
    AudioJob job;
    job.type      = AudioJob::JOB_CHIRP;
    job.startFreq = startFreq;
    job.endFreq   = endFreq;
    job.durationMs = durationMs;
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::playPhonemes(int wordCount, const char* subtitle) {
    AudioJob job;
    job.type      = AudioJob::JOB_PHONEME;
    job.wordCount = wordCount;
    if (subtitle) strncpy(job.subtitle, subtitle, 63);
    else job.subtitle[0] = '\0';
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::_audioTask(void* param) {
    AudioEngine* self = static_cast<AudioEngine*>(param);
    AudioJob job;
    for (;;) {
        if (xQueueReceive(self->_queue, &job, portMAX_DELAY) == pdTRUE) {
            int vol = self->_muted ? 0 : self->_volume;
            switch (job.type) {
                case AudioJob::JOB_HD_SAMPLE:
                    self->currentMouthShape = job.mouthShape;
                    self->_playHDInternal(job.data, job.dataLen, vol);
                    self->currentMouthShape = -1;
                    break;
                case AudioJob::JOB_CHIRP:
                    self->_playChirpInternal(job.startFreq, job.endFreq, job.durationMs, vol);
                    break;
                case AudioJob::JOB_PHONEME:
                    self->currentMouthShape = 0;
                    self->_playPhonemesInternal(job.wordCount, job.subtitle, vol);
                    self->currentMouthShape = -1;
                    break;
            }
        }
    }
}

void AudioEngine::_playHDInternal(const uint8_t* data, int len, int vol) {
    micMuteUntil = millis() + (len / VOICE_SAMPLE_RATE * 1000) + 1500;
    if (vol == 0) { delay(300); return; }
    int delayUs = 1000000 / VOICE_SAMPLE_RATE;
    for (int i = 0; i < len; i++) {
        uint8_t raw = pgm_read_byte(&data[i]);
        int centered = (int)raw - 128;
        int scaled = (centered * vol) / 100;
        dacWrite(AUDIO_DAC_PIN, (uint8_t)constrain(scaled + 128, 0, 255));
        delayMicroseconds(delayUs);
    }
    for (int fade = 128; fade >= 0; fade -= 8) {
        dacWrite(AUDIO_DAC_PIN, (uint8_t)((fade * vol) / 100));
        delayMicroseconds(120);
    }
    dacWrite(AUDIO_DAC_PIN, 0);
}

void AudioEngine::_playChirpInternal(int sf, int ef, int dur, int vol) {
    micMuteUntil = millis() + dur + 1000;
    if (vol == 0) { delay(dur); return; }
    int steps = dur * 5;
    for (int i = 0; i < steps; i++) {
        float p    = (float)i / steps;
        int   freq = max(200, sf + (int)((ef - sf) * p));
        int   half = 500000 / freq;
        uint8_t hi = (uint8_t)constrain(128 + (vol * 50) / 100, 0, 255);
        uint8_t lo = (uint8_t)constrain(128 - (vol * 50) / 100, 0, 255);
        dacWrite(AUDIO_DAC_PIN, hi); delayMicroseconds(half);
        dacWrite(AUDIO_DAC_PIN, lo); delayMicroseconds(half);
    }
    dacWrite(AUDIO_DAC_PIN, 0);
}

void AudioEngine::_playPhonemesInternal(int words, const char* sub, int vol) {
    int syllables = constrain(words * 2, 4, 16);
    micMuteUntil = millis() + (syllables * 120) + 1500;
    if (vol == 0) { delay(syllables * 120); return; }
    for (int s = 0; s < syllables; s++) {
        currentMouthShape = s % 4;
        int base = random(600, 1100);
        int tgt  = base + random(-200, 250);
        int dur  = random(50, 90);
        int steps = dur * 4;
        for (int i = 0; i < steps; i++) {
            float p    = (float)i / steps;
            int   freq = max(300, base + (int)((tgt - base) * p));
            int   half = 500000 / freq;
            uint8_t hi = (uint8_t)constrain(128 + (vol * 45) / 100, 0, 255);
            uint8_t lo = (uint8_t)constrain(128 - (vol * 45) / 100, 0, 255);
            dacWrite(AUDIO_DAC_PIN, hi); delayMicroseconds(half);
            dacWrite(AUDIO_DAC_PIN, lo); delayMicroseconds(half);
        }
        dacWrite(AUDIO_DAC_PIN, 0);
        delay(random(15, 35));
    }
    currentMouthShape = -1;
    dacWrite(AUDIO_DAC_PIN, 0);
}
```

- [ ] **Step 3: Verify compiles**

Build with `pio run`. Expected: clean. If `xQueueCreate` not found, ensure `#include <freertos/FreeRTOS.h>` is included (usually implicit with esp32 Arduino).

- [ ] **Step 4: Commit**

```bash
git add Piku/AudioEngine.h Piku/AudioEngine.cpp
git commit -m "feat(piku2): AudioEngine with FreeRTOS non-blocking DAC queue"
```

---

## Task 3: SensorEngine — Touch + Clap Detection

**Files:**
- Create: `Piku/SensorEngine.h`
- Create: `Piku/SensorEngine.cpp`

**Interfaces:**
- Consumes: `AudioEngine::micMuteUntil`
- Produces: `SensorEngine::init()`, `SensorEngine::update()`, callback setters: `SensorEngine::onTouchShort(cb)`, `SensorEngine::onTouchSustained(cb)`, `SensorEngine::onTouchOverpet(cb)`, `SensorEngine::onDoubleClap(cb)`, `SensorEngine::setSoundEnabled(bool)`

- [ ] **Step 1: Create SensorEngine.h**

```cpp
#pragma once
#include <Arduino.h>
#include "config.h"
#include "AudioEngine.h"

class SensorEngine {
public:
    using Callback = void(*)();
    void init(AudioEngine* audio);
    void update();
    void setSoundEnabled(bool en) { _soundEnabled = en; }
    bool getSoundEnabled()  const { return _soundEnabled; }
    void onTouchShort(Callback cb)     { _cbTouchShort = cb; }
    void onTouchSustained(Callback cb) { _cbTouchSustained = cb; }
    void onTouchOverpet(Callback cb)   { _cbTouchOverpet = cb; }
    void onDoubleClap(Callback cb)     { _cbDoubleClap = cb; }

private:
    AudioEngine* _audio;
    bool _soundEnabled = false;

    // Touch state
    bool          _petting          = false;
    unsigned long _petStart         = 0;
    unsigned long _lastPetTick      = 0;
    int           _touchDebounce    = 0;
    Callback      _cbTouchShort     = nullptr;
    Callback      _cbTouchSustained = nullptr;
    Callback      _cbTouchOverpet   = nullptr;

    // Clap state
    int           _clapCount        = 0;
    unsigned long _firstClapTime    = 0;
    Callback      _cbDoubleClap     = nullptr;

    void _updateTouch();
    void _updateClap();
};
```

- [ ] **Step 2: Create SensorEngine.cpp**

```cpp
#include "SensorEngine.h"

void SensorEngine::init(AudioEngine* audio) {
    _audio = audio;
#if ENABLE_SOUND_SENSOR
    pinMode(MIC_DO_PIN, INPUT);
#endif
}

void SensorEngine::update() {
    _updateTouch();
    _updateClap();
}

void SensorEngine::_updateTouch() {
#if ENABLE_TOUCH_PIN
    int val = touchRead(TOUCH_HEAD_PIN);
    unsigned long now = millis();
    if (val < 35 && val > 0) {
        _touchDebounce++;
        if (_touchDebounce >= 2) {
            if (!_petting) {
                _petting   = true;
                _petStart  = now;
                _lastPetTick = now;
            }
            unsigned long dur = now - _petStart;
            if (dur > 5500) {
                if (now - _lastPetTick > 1800) {
                    _lastPetTick = now;
                    if (_cbTouchOverpet) _cbTouchOverpet();
                }
            } else if (dur > 1200) {
                if (now - _lastPetTick > 2200) {
                    _lastPetTick = now;
                    if (_cbTouchSustained) _cbTouchSustained();
                }
            }
        }
    } else {
        _touchDebounce = 0;
        if (_petting) {
            unsigned long dur = now - _petStart;
            _petting = false;
            if (dur < 1200) {
                if (_cbTouchShort) _cbTouchShort();
            }
        }
    }
#endif
}

void SensorEngine::_updateClap() {
#if ENABLE_SOUND_SENSOR
    if (!_soundEnabled) return;
    unsigned long now = millis();
    if (now < _audio->micMuteUntil) return;
    if (_clapCount > 0 && (now - _firstClapTime > 650)) _clapCount = 0;

    if (digitalRead(MIC_DO_PIN) == LOW) {
        if (_clapCount == 0) {
            _clapCount = 1;
            _firstClapTime = now;
            _audio->micMuteUntil = now + 120;
        } else if (_clapCount == 1 &&
                   (now - _firstClapTime >= 150) &&
                   (now - _firstClapTime <= 600)) {
            _clapCount = 0;
            _audio->micMuteUntil = now + 4000;
            if (_cbDoubleClap) _cbDoubleClap();
        }
    }
#endif
}
```

- [ ] **Step 3: Verify compiles**

Build with `pio run`. Expected: clean.

- [ ] **Step 4: Commit**

```bash
git add Piku/SensorEngine.h Piku/SensorEngine.cpp
git commit -m "feat(piku2): SensorEngine with callback-based touch and clap events"
```

---

## Task 4: DisplayEngine — Parametric Smooth OLED Animator

**Files:**
- Create: `Piku/DisplayEngine.h`
- Create: `Piku/DisplayEngine.cpp`

**Interfaces:**
- Consumes: `AudioEngine::currentMouthShape`
- Produces: `DisplayEngine::init()`, `DisplayEngine::morphToEmotion(RobotEmotion e, int durationMs)`, `DisplayEngine::update()` (call every 33ms from soul loop), `DisplayEngine::startScrollMessage(const String& text, const char* title)`, `DisplayEngine::showVolumeHUD(int vol, bool muted)`

All 24 `RobotEmotion` values from current code preserved exactly.

- [ ] **Step 1: Create DisplayEngine.h**

```cpp
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

    // Overlay tick counter
    unsigned long _overlayFrame = 0;

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
```

- [ ] **Step 2: Create DisplayEngine.cpp (part 1 — init + morph + eye draw)**

```cpp
#include "DisplayEngine.h"

void DisplayEngine::init() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);
    _disp = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (_disp.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        _ready = true;
        _disp.clearDisplay();
        _disp.setTextSize(2);
        _disp.setTextColor(SSD1306_WHITE);
        _disp.setCursor(24, 20);
        _disp.print(F("PIKU AI"));
        _disp.setTextSize(1);
        _disp.setCursor(14, 44);
        _disp.print(F("WAKING UP..."));
        _disp.display();
    }
    _current = _targetShapeFor(EMOTION_IDLE);
    _from    = _current;
    _to      = _current;
    _nextBlink = millis() + random(3000, 6000);
}

void DisplayEngine::morphToEmotion(RobotEmotion e, int durationMs) {
    if (_currentEmotion == e) return;
    _currentEmotion = e;
    _from           = _current;
    _to             = _targetShapeFor(e);
    _morphStart     = millis();
    _morphDuration  = durationMs;
}

EyeShape DisplayEngine::_lerpShape(const EyeShape& a, const EyeShape& b, float t) {
    EyeShape s;
    s.wL       = _lerp(a.wL, b.wL, t);
    s.hL       = _lerp(a.hL, b.hL, t);
    s.rL       = _lerp(a.rL, b.rL, t);
    s.wR       = _lerp(a.wR, b.wR, t);
    s.hR       = _lerp(a.hR, b.hR, t);
    s.rR       = _lerp(a.rR, b.rR, t);
    s.pupilSize = _lerp(a.pupilSize, b.pupilSize, t);
    s.offsetY  = _lerp(a.offsetY, b.offsetY, t);
    s.winkLeft = b.winkLeft;
    return s;
}

// Target EyeShape for each emotion
EyeShape DisplayEngine::_targetShapeFor(RobotEmotion e) {
    // All values: {wL, hL, rL, wR, hR, rR, pupilSize, offsetY, winkLeft}
    switch (e) {
        case EMOTION_IDLE:             return {36, 40, 10, 36, 40, 10, 6, 0,    false};
        case EMOTION_HELLO:            return {36, 44, 10, 36, 44, 10, 7, -4,   false};
        case EMOTION_LOVE:             return {32, 32, 16, 32, 32, 16, 0, 0,    false}; // hearts rendered in overlay
        case EMOTION_TADA:             return {38, 46, 10, 38, 46, 10, 8, -6,   false};
        case EMOTION_PARTY_DJ:         return {36, 36, 4,  36, 36, 4,  0, 0,    false}; // EQ bars overlay
        case EMOTION_CURIOUS_SCAN:     return {30, 42, 8,  30, 42, 8,  5, -3,   false};
        case EMOTION_UHOH_ALERT:       return {28, 28, 6,  28, 28, 6,  4, 2,    false};
        case EMOTION_COOL_SUNGLASSES:  return {44, 32, 6,  44, 32, 6,  0, 0,    false};
        case EMOTION_SLEEP:            return {36, 4,  2,  36, 4,  2,  0, 4,    false};
        case EMOTION_GAMER_PACMAN:     return {28, 28, 14, 28, 28, 14, 5, 0,    false};
        case EMOTION_RAINY_SAD:        return {32, 14, 4,  32, 14, 4,  3, 4,    false};
        case EMOTION_FIRE_RAGE:        return {36, 32, 2,  36, 32, 2,  4, 0,    false};
        case EMOTION_HYPNO_DIZZY:      return {32, 32, 16, 32, 32, 16, 8, 0,    false};
        case EMOTION_KAWAII_CAT:       return {28, 20, 6,  28, 20, 6,  3, 2,    false};
        case EMOTION_JACKPOT_MONEY:    return {36, 36, 4,  36, 36, 4,  0, 0,    false};
        case EMOTION_MATRIX_HACKER:    return {30, 28, 2,  30, 28, 2,  3, 0,    false};
        case EMOTION_KAWAII_KISS:      return {28, 28, 14, 36, 40, 10, 6, 0,    true };
        case EMOTION_FOCUS_STUDY:      return {36, 28, 2,  36, 28, 2,  4, 0,    false};
        case EMOTION_MAGIC_8BALL:      return {28, 36, 8,  28, 36, 8,  5, 0,    false};
        case EMOTION_CLOCK_DISPLAY:    return {24, 16, 4,  24, 16, 4,  3, -8,   false};
        case EMOTION_WEATHER_DISPLAY:  return {24, 16, 4,  24, 16, 4,  3, -8,   false};
        case EMOTION_SENTRY_ALERT:     return {36, 36, 2,  36, 36, 2,  5, 0,    false};
        case EMOTION_SNACK_EAT:        return {32, 36, 6,  32, 36, 6,  6, 0,    false};
        case EMOTION_RPS_SHOW:         return {28, 28, 8,  28, 28, 8,  4, 0,    false};
        default:                       return {36, 40, 10, 36, 40, 10, 6, 0,    false};
    }
}

void DisplayEngine::triggerBlink() {
    _blinking  = true;
    _blinkProg = 0.0f;
}
```

- [ ] **Step 3: Create DisplayEngine.cpp (part 2 — update + draw)**

Append to DisplayEngine.cpp:

```cpp
void DisplayEngine::update(AudioEngine* audio) {
    if (!_ready) return;
    _overlayFrame = millis();

    // Advance morph
    unsigned long now = millis();
    float t = 1.0f;
    if (_morphDuration > 0) {
        t = (float)(now - _morphStart) / (float)_morphDuration;
        t = constrain(t, 0.0f, 1.0f);
        // Ease in-out cubic
        t = t < 0.5f ? 4*t*t*t : 1 - pow(-2*t + 2, 3) / 2;
    }
    _current = _lerpShape(_from, _to, t);

    // Advance blink
    float openRatio = 1.0f;
    if (_blinking) {
        _blinkProg += 0.25f;
        if (_blinkProg >= 1.0f) { _blinking = false; _blinkProg = 0; _nextBlink = now + random(3000, 8000); }
        openRatio = 1.0f - sinf(_blinkProg * M_PI);
    } else if (now >= _nextBlink) {
        triggerBlink();
    }

    if (_scrolling) { _updateScroll(); return; }

    // Handle audio mouth sync
    int mouth = audio ? audio->currentMouthShape : -1;

    // Check for special full-screen emotions
    bool special = (_currentEmotion == EMOTION_PARTY_DJ ||
                    _currentEmotion == EMOTION_SENTRY_ALERT ||
                    _currentEmotion == EMOTION_GAMER_PACMAN ||
                    _currentEmotion == EMOTION_MATRIX_HACKER ||
                    _currentEmotion == EMOTION_CURIOUS_SCAN ||
                    _currentEmotion == EMOTION_CLOCK_DISPLAY ||
                    _currentEmotion == EMOTION_WEATHER_DISPLAY);

    _disp.clearDisplay();

    if (mouth >= 0) {
        // During audio: show speech face
        _renderSpeechFace(mouth, nullptr);
    } else if (special) {
        _renderSpecial();
    } else {
        _renderIdle(_gazeX, _gazeY, openRatio);
        _drawOverlay(_currentEmotion);
    }
    _disp.display();
}

void DisplayEngine::_renderIdle(float gx, float gy, float openRatio) {
    EyeShape s = _current;
    float hL = s.hL * openRatio;
    float hR = (s.winkLeft ? 2.0f : s.hR * openRatio);
    if (hL < 2) hL = 2;
    if (hR < 2) hR = 2;

    int lx = EYE_L_CX + (int)gx;
    int rx = EYE_R_CX + (int)gx;
    int cy = EYE_CY   + (int)(gy + s.offsetY);

    _disp.fillRoundRect(lx - (int)(s.wL/2), cy - (int)(hL/2), (int)s.wL, (int)hL, (int)s.rL, SSD1306_WHITE);
    _disp.fillRoundRect(rx - (int)(s.wR/2), cy - (int)(hR/2), (int)s.wR, (int)hR, (int)s.rR, SSD1306_WHITE);

    if (openRatio > 0.45f && s.pupilSize > 0) {
        int px = (int)(gx * 0.45f);
        int py = (int)(gy * 0.45f);
        int pr = (int)s.pupilSize;
        _disp.fillCircle(lx + px, cy + py, pr, SSD1306_BLACK);
        _disp.fillCircle(rx + px, cy + py, pr, SSD1306_BLACK);
        _disp.fillCircle(lx + px + 2, cy + py - 2, max(1, pr/3), SSD1306_WHITE);
        _disp.fillCircle(rx + px + 2, cy + py - 2, max(1, pr/3), SSD1306_WHITE);
    }
}

void DisplayEngine::_drawHeart(int cx, int cy, int size) {
    int r = size / 4;
    _disp.fillCircle(cx - r, cy - r, r, SSD1306_WHITE);
    _disp.fillCircle(cx + r, cy - r, r, SSD1306_WHITE);
    _disp.fillTriangle(cx - size/2, cy - r + 1, cx + size/2, cy - r + 1, cx, cy + size/2, SSD1306_WHITE);
}

void DisplayEngine::_drawOverlay(RobotEmotion e) {
    int f = (int)(_overlayFrame / 80) % 8;
    switch (e) {
        case EMOTION_LOVE:
        case EMOTION_KAWAII_KISS:
            _drawHeart(EYE_L_CX, EYE_CY, 30);
            _drawHeart(EYE_R_CX, EYE_CY, 30);
            break;
        case EMOTION_KAWAII_CAT:
            // Cat ears
            _disp.fillTriangle(EYE_L_CX - 16, EYE_CY - 16, EYE_L_CX, EYE_CY - 16,
                               EYE_L_CX - 8, EYE_CY - 30, SSD1306_WHITE);
            _disp.fillTriangle(EYE_R_CX, EYE_CY - 16, EYE_R_CX + 16, EYE_CY - 16,
                               EYE_R_CX + 8, EYE_CY - 30, SSD1306_WHITE);
            // Blush
            for (int i = -3; i <= 3; i += 2) {
                _disp.drawPixel(EYE_L_CX - 12 + i, EYE_CY + 12, SSD1306_WHITE);
                _disp.drawPixel(EYE_R_CX + 10 + i, EYE_CY + 12, SSD1306_WHITE);
            }
            break;
        case EMOTION_SLEEP:
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            if (f >= 0) { _disp.setCursor(80, 26); _disp.print(F("z")); }
            if (f >= 3) { _disp.setCursor(92, 16); _disp.print(F("Z")); }
            if (f >= 6) { _disp.setCursor(106, 6); _disp.print(F("Z")); }
            break;
        case EMOTION_RAINY_SAD:
            for (int i = 0; i < 6; i++) {
                int rx = 12 + i * 20;
                int ry = ((int)(_overlayFrame / 5) + i * 14) % 40 + 10;
                _disp.drawLine(rx, ry, rx - 2, ry + 6, SSD1306_WHITE);
            }
            break;
        case EMOTION_FIRE_RAGE: {
            int fs = (int)(_overlayFrame / 40) % 12;
            for (int i = 0; i < 8; i++) {
                int fx = 12 + i * 14;
                int fh = 6 + (fs + i * 3) % 10;
                _disp.fillTriangle(fx - 4, 16, fx + 4, 16, fx, 16 - fh, SSD1306_WHITE);
            }
            break;
        }
        default: break;
    }
}

void DisplayEngine::_drawMouth(int shape) {
    switch (shape) {
        case 0: _disp.drawFastHLine(56, 50, 16, SSD1306_WHITE); break;
        case 1: _disp.drawCircle(64, 50, 4, SSD1306_WHITE); break;
        case 2: _disp.fillRoundRect(56, 46, 16, 8, 3, SSD1306_WHITE);
                _disp.fillRoundRect(58, 47, 12, 6, 2, SSD1306_BLACK); break;
        case 3: _disp.fillRoundRect(52, 44, 24, 12, 5, SSD1306_WHITE);
                _disp.fillCircle(64, 46, 5, SSD1306_BLACK); break;
        default: _disp.drawCircle(64, 48, 7, SSD1306_WHITE);
                 _disp.fillRect(52, 41, 24, 7, SSD1306_BLACK); break;
    }
}

void DisplayEngine::_renderSpeechFace(int mouthShape, const char* subtitle) {
    EyeShape neutral = _targetShapeFor(EMOTION_HELLO);
    _renderIdle(0, 0, 1.0f);
    _drawMouth(mouthShape);
    if (subtitle) {
        _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
        int len = strlen(subtitle);
        int sx  = (128 - len * 6) / 2;
        _disp.setCursor(max(2, sx), 57);
        _disp.print(subtitle);
    }
}

void DisplayEngine::_renderSpecial() {
    int f = (int)(_overlayFrame / 30) % 64;
    switch (_currentEmotion) {
        case EMOTION_PARTY_DJ: {
            int b1 = 8 + f % 24, b2 = 8 + (f + 12) % 24;
            _disp.fillRoundRect(EYE_L_CX - 16, EYE_CY - b1/2, 32, b1, 4, SSD1306_WHITE);
            _disp.fillRoundRect(EYE_R_CX - 16, EYE_CY - b2/2, 32, b2, 4, SSD1306_WHITE);
            _disp.fillRect(64 - f%20/2, 54, f%20, 4, SSD1306_WHITE);
            break;
        }
        case EMOTION_SENTRY_ALERT:
            if ((f / 8) % 2 == 0) { _disp.fillRect(0,0,128,64,SSD1306_WHITE); _disp.setTextColor(SSD1306_BLACK); }
            else _disp.setTextColor(SSD1306_WHITE);
            _disp.setTextSize(2); _disp.setCursor(8, 14); _disp.print(F("SENTRY"));
            _disp.setTextSize(1); _disp.setCursor(8, 42); _disp.print(F("INTRUDER DETECTED!"));
            break;
        case EMOTION_MATRIX_HACKER:
            for (int col = 6; col < 124; col += 12) {
                int sy = (f * 3 + col * 7) % 50;
                _disp.drawFastVLine(col, sy, 8, SSD1306_WHITE);
            }
            _disp.drawRect(34, 14, 60, 36, SSD1306_WHITE);
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(22, 55); _disp.print(F("[ ACCESS GRANTED ]"));
            break;
        case EMOTION_CURIOUS_SCAN: {
            int sy = 12 + f % 40;
            _disp.drawRoundRect(8, 8, 112, 48, 6, SSD1306_WHITE);
            _disp.drawFastHLine(4, 32, 120, SSD1306_WHITE);
            _disp.drawFastVLine(64, 4, 56, SSD1306_WHITE);
            _disp.drawFastHLine(12, sy, 104, SSD1306_WHITE);
            _disp.fillCircle(64, sy, 3, SSD1306_WHITE);
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(14, 54); _disp.print(F("AI THINKING..."));
            break;
        }
        default: _renderIdle(_gazeX, _gazeY, 1.0f); break;
    }
}

void DisplayEngine::_updateScroll() {
    unsigned long now = millis();
    if (now < _nextScrollTick) { _disp.display(); return; }
    _nextScrollTick = now + 25;
    _scrollX -= 3;
    int totalLen = (int)(_scrollText.length() * 6);
    if (_scrollX < -totalLen) { _scrolling = false; return; }

    _disp.clearDisplay();
    _disp.fillRoundRect(EYE_L_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);
    if (_scrollTitle.length() > 0) {
        _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
        _disp.setCursor(4, 26); _disp.print(_scrollTitle);
    }
    _disp.drawRoundRect(2, 36, 124, 26, 4, SSD1306_WHITE);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(_scrollX, 46); _disp.print(_scrollText);
    _disp.display();
}

void DisplayEngine::startScrollMessage(const String& text, const char* title) {
    _scrollText  = text;
    _scrollTitle = (title) ? String(title) : "";
    _scrollX     = 128;
    _scrolling   = true;
    _nextScrollTick = millis();
}

void DisplayEngine::showVolumeHUD(int vol, bool muted) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(38, 10); _disp.print(F("VOLUME"));
    int fillW = muted ? 0 : (vol * 96) / 100;
    _disp.drawRoundRect(16, 28, 96, 16, 4, SSD1306_WHITE);
    if (fillW > 0) _disp.fillRoundRect(16, 28, fillW, 16, 4, SSD1306_WHITE);
    _disp.setCursor(30, 50);
    if (muted || vol == 0) _disp.print(F("MUTED"));
    else { _disp.print(vol); _disp.print(F("%")); }
    _disp.display();
}

// Mini-game + special screen renderers
void DisplayEngine::renderFlappyGame(int bY, float vel, int score, int hi, int pX, int pGY, bool over) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.fillCircle(24, bY, 6, SSD1306_WHITE);
    _disp.fillCircle(26, bY - 2, 2, SSD1306_BLACK);
    _disp.fillTriangle(30, bY - 1, 35, bY + 1, 30, bY + 3, SSD1306_WHITE);
    _disp.fillRect(pX, 0, 16, pGY, SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY - 4, 20, 4, SSD1306_WHITE);
    _disp.fillRect(pX, pGY + 28, 16, 64 - (pGY + 28), SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY + 28, 20, 4, SSD1306_WHITE);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(4, 2); _disp.print(F("SCORE:")); _disp.print(score);
    _disp.setCursor(76, 2); _disp.print(F("HI:")); _disp.print(hi);
    if (over) {
        _disp.fillRoundRect(16, 20, 96, 26, 4, SSD1306_BLACK);
        _disp.drawRoundRect(16, 20, 96, 26, 4, SSD1306_WHITE);
        _disp.setCursor(24, 25); _disp.print(F("GAME OVER!"));
        _disp.setCursor(20, 35); _disp.print(F("Tap to Retry"));
    }
    _disp.display();
}

void DisplayEngine::renderRPS(const char* choice) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(14, 4); _disp.print(F("ROCK PAPER SCISSORS"));
    _disp.drawRoundRect(24, 18, 80, 42, 6, SSD1306_WHITE);
    _disp.setTextSize(2);
    _disp.setCursor(64 - strlen(choice) * 6, 30);
    _disp.print(choice);
    _disp.display();
}

void DisplayEngine::renderSentryAlert(int tick) {
    _currentEmotion = EMOTION_SENTRY_ALERT;
    update(nullptr);
}

void DisplayEngine::renderSnackEat(int frame) {
    if (!_ready) return;
    _disp.clearDisplay();
    int h = (frame % 2 == 0) ? 36 : 14;
    _disp.fillRoundRect(EYE_L_CX - 16, EYE_CY - h/2, 32, h, 6, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 16, EYE_CY - h/2, 32, h, 6, SSD1306_WHITE);
    _disp.fillCircle(64, 50, (frame%2==0)?10:3, SSD1306_WHITE);
    _disp.display();
}

void DisplayEngine::renderMagic8Ball(const char* answer) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.drawCircle(64, 30, 24, SSD1306_WHITE);
    _disp.fillCircle(64, 30, 14, SSD1306_WHITE);
    _disp.fillTriangle(54, 36, 74, 36, 64, 20, SSD1306_BLACK);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    int len = strlen(answer);
    _disp.setCursor(max(4, (128 - len*6)/2), 58);
    _disp.print(answer);
    _disp.display();
}

void DisplayEngine::renderClockScreen(const char* timeStr, const char* dateStr) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.fillRoundRect(EYE_L_CX - 12, 4, 24, 16, 4, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 12, 4, 24, 16, 4, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);
    _disp.drawRoundRect(4, 24, 120, 38, 5, SSD1306_WHITE);
    _disp.setTextSize(2); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(64 - strlen(timeStr)*6, 28); _disp.print(timeStr);
    _disp.setTextSize(1);
    _disp.setCursor(64 - strlen(dateStr)*3, 48); _disp.print(dateStr);
    _disp.display();
}

void DisplayEngine::renderWeatherScreen(int tempC, int humidity, const char* condition) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.drawRoundRect(2, 2, 124, 60, 6, SSD1306_WHITE);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(8, 8); _disp.print(F("LIVE WEATHER"));
    _disp.setTextSize(3); _disp.setCursor(10, 22);
    _disp.print(tempC); _disp.setTextSize(1); _disp.print(F("C"));
    _disp.setCursor(80, 24); _disp.print(F("HUM:"));
    _disp.setCursor(80, 34); _disp.print(humidity); _disp.print(F("%"));
    _disp.setCursor(8, 48); _disp.print(condition);
    _disp.display();
}
```

- [ ] **Step 4: Verify compiles**

Build with `pio run`. Expected: clean. If `sinf` not found add `#include <math.h>`.

- [ ] **Step 5: Commit**

```bash
git add Piku/DisplayEngine.h Piku/DisplayEngine.cpp
git commit -m "feat(piku2): DisplayEngine with parametric smooth OLED keyframe animation"
```

---

## Task 5: SoulEngine — Autonomous Behaviour Scheduler + Emotional State

**Files:**
- Create: `Piku/SoulEngine.h`
- Create: `Piku/SoulEngine.cpp`

**Interfaces:**
- Consumes: `DisplayEngine::morphToEmotion()`, `ServoEngine::performGesture()`, `ServoEngine::setTarget()`, `AudioEngine::playHD/Chirp/Phonemes()`
- Produces: `SoulEngine::init(...)`, `SoulEngine::update()` (call from Core 1 loop every 33ms), `SoulEngine::onTouchShort()`, `SoulEngine::onTouchSustained()`, `SoulEngine::onTouchOverpet()`, `SoulEngine::onDoubleClap()`, `SoulEngine::triggerEmotion(RobotEmotion, int intensity, int durationMs)`, `SoulEngine::requestAITalk(const char* topic)`, `SoulEngine::setAutoTalkInterval(int minutes)`, `SoulEngine::getState()`, `SoulEngine::getEmotion()`, `SoulEngine::getAffection()`, `SoulEngine::getEnergy()`, `SoulEngine::getHunger()`

Enum `CompanionState` preserved: `STATE_AWAKE_IDLE, STATE_HAPPY_AFFECTION, STATE_DROWSY_NAP, STATE_DEEP_SLEEP, STATE_GAME_FLAPPY, STATE_GAME_RPS, STATE_SENTRY_GUARD, STATE_FOCUS_STUDY, STATE_SNACK_FEEDING, STATE_AI_THINKING, STATE_AI_SPEAKING`

- [ ] **Step 1: Create SoulEngine.h**

```cpp
#pragma once
#include <Arduino.h>
#include "DisplayEngine.h"
#include "AudioEngine.h"
#include "ServoEngine.h"
#include "voice_samples.h"
#include "config.h"

enum CompanionState {
    STATE_AWAKE_IDLE, STATE_HAPPY_AFFECTION, STATE_DROWSY_NAP, STATE_DEEP_SLEEP,
    STATE_GAME_FLAPPY, STATE_GAME_RPS, STATE_SENTRY_GUARD, STATE_FOCUS_STUDY,
    STATE_SNACK_FEEDING, STATE_AI_THINKING, STATE_AI_SPEAKING
};

class SoulEngine {
public:
    void init(DisplayEngine* disp, AudioEngine* audio, ServoEngine* servo);
    void update();   // call every 33ms from Core 1

    // Sensor event hooks (called from SensorEngine callbacks)
    void onTouchShort();
    void onTouchSustained();
    void onTouchOverpet();
    void onDoubleClap();

    // BrainEngine calls this after getting AI response
    void triggerEmotion(RobotEmotion e, int intensity, int durationMs = 4000);
    void onAIResponseReceived(RobotEmotion e, int intensity, const String& text);
    void setAIThinking(bool thinking);

    // Autonomous talk request — BrainEngine reads this
    bool hasPendingAIRequest() const { return _pendingAIRequest; }
    String consumeAIRequest()        { _pendingAIRequest = false; return _pendingAITopic; }

    void setAutoTalkInterval(int minutes) { _autoTalkIntervalMs = (unsigned long)minutes * 60000UL; }
    int  getAutoTalkIntervalMinutes() const { return (int)(_autoTalkIntervalMs / 60000UL); }

    CompanionState getState()    const { return _state; }
    RobotEmotion   getEmotion()  const { return _emotion; }
    int  getAffection()          const { return _affection; }
    int  getEnergy()             const { return _energy; }
    int  getHunger()             const { return _hunger; }
    void feedSnack()   { _hunger = min(100, _hunger + 25); _affection = min(100, _affection + 5); }
    void setState(CompanionState s) { _state = s; }

    // Gaze output for DisplayEngine
    float gazeX = 0, gazeY = 0;

private:
    DisplayEngine* _disp;
    AudioEngine*   _audio;
    ServoEngine*   _servo;

    CompanionState _state     = STATE_AWAKE_IDLE;
    RobotEmotion   _emotion   = EMOTION_IDLE;
    int _affection = 85, _energy = 100, _hunger = 90;
    unsigned long _lastInteraction    = 0;
    unsigned long _emotionResetTime   = 0;
    unsigned long _nextMetabolism     = 0;
    unsigned long _nextGazeShift      = 0;
    float _targetGazeX = 0, _targetGazeY = 0;

    // Auto-talk
    unsigned long _autoTalkIntervalMs = 10UL * 60000UL; // 10 minutes default
    unsigned long _nextAutoTalk       = 0;
    bool   _pendingAIRequest  = false;
    String _pendingAITopic;

    // Clap cooldown
    unsigned long _clapCoolUntil = 0;

    void _triggerVoiceForEmotion(RobotEmotion e);
    void _doMetabolismTick();
    void _doGazeUpdate();
    void _doAutoTalkCheck();
    void _doEmotionResetCheck();

    static const char* _pickRandomTopic();
};
```

- [ ] **Step 2: Create SoulEngine.cpp**

```cpp
#include "SoulEngine.h"

void SoulEngine::init(DisplayEngine* disp, AudioEngine* audio, ServoEngine* servo) {
    _disp  = disp;
    _audio = audio;
    _servo = servo;
    _lastInteraction = millis();
    _nextAutoTalk    = millis() + _autoTalkIntervalMs;
    _nextMetabolism  = millis() + 45000;
    _nextGazeShift   = millis() + random(2000, 5000);
}

void SoulEngine::update() {
    _doGazeUpdate();
    _doEmotionResetCheck();
    _doMetabolismTick();
    _doAutoTalkCheck();
    _disp->setGaze(gazeX, gazeY);
}

void SoulEngine::_doGazeUpdate() {
    unsigned long now = millis();
    if (now >= _nextGazeShift) {
        _nextGazeShift = now + random(2200, 5500);
        _targetGazeX = random(-8, 9);
        _targetGazeY = random(-4, 5);
        if (random(0, 4) == 0) {
            _servo->setTarget(SERVO_CENTER + _targetGazeX * 2.2f);
        }
    }
    gazeX += (_targetGazeX - gazeX) * 0.25f;
    gazeY += (_targetGazeY - gazeY) * 0.25f;
}

void SoulEngine::_doEmotionResetCheck() {
    if (_emotion != EMOTION_IDLE && _emotionResetTime > 0 && millis() >= _emotionResetTime) {
        _emotion = EMOTION_IDLE;
        _state   = STATE_AWAKE_IDLE;
        _emotionResetTime = 0;
        _servo->setTarget(SERVO_CENTER);
        _disp->morphToEmotion(EMOTION_IDLE, 400);
    }
}

void SoulEngine::_doMetabolismTick() {
    unsigned long now = millis();
    if (now < _nextMetabolism) return;
    _nextMetabolism = now + 45000;

    if (_state == STATE_AWAKE_IDLE) {
        _energy  = max(0, _energy - 1);
        _hunger  = max(0, _hunger - 1);
        if (_hunger < 20 && _emotion == EMOTION_IDLE && now - _lastInteraction > 120000) {
            triggerEmotion(EMOTION_RAINY_SAD, 60, 3500);
        }
        if (_energy < 10) {
            _state  = STATE_DEEP_SLEEP;
            _emotion = EMOTION_SLEEP;
            _disp->morphToEmotion(EMOTION_SLEEP, 600);
            _servo->setTarget(SERVO_CENTER - 20.0f);
            _audio->playHD(voice_sleep_data, sizeof(voice_sleep_data), 0, "Zzz...");
        }
    } else if (_state == STATE_DEEP_SLEEP) {
        _energy = min(100, _energy + 5);
        if (_energy >= 80) {
            _state  = STATE_AWAKE_IDLE;
            _emotion = EMOTION_IDLE;
            _disp->morphToEmotion(EMOTION_HELLO, 400);
            _servo->setTarget(SERVO_CENTER);
            _audio->playHD(voice_hello_data, sizeof(voice_hello_data), 2, "Good morning!");
            _emotionResetTime = millis() + 3000;
        }
    }
}

void SoulEngine::_doAutoTalkCheck() {
    if (_autoTalkIntervalMs == 0) return;
    unsigned long now = millis();
    if (now < _nextAutoTalk) return;
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) return;
    if (_pendingAIRequest) return;

    _nextAutoTalk    = now + _autoTalkIntervalMs;
    _pendingAIRequest = true;
    _pendingAITopic   = String(_pickRandomTopic());
}

const char* SoulEngine::_pickRandomTopic() {
    static const char* topics[] = {
        "Comment on the current time of day in a funny way.",
        "Say something about the weather right now.",
        "Ask the owner how they are doing today.",
        "Share one short interesting fun fact about anything.",
        "Say something playful and random to cheer the owner up.",
        "Ask what the owner is working on right now.",
        "Say something witty about being a desk robot.",
    };
    return topics[random(0, 7)];
}

void SoulEngine::triggerEmotion(RobotEmotion e, int intensity, int durationMs) {
    _emotion = e;
    _emotionResetTime = millis() + durationMs;
    _disp->morphToEmotion(e, 300);
    if (intensity >= 80) _servo->triggerWiggle();
    else if (intensity >= 50) {
        float offset = (intensity > 65) ? 18.0f : 10.0f;
        _servo->setTarget(SERVO_CENTER + (random(0,2)==0 ? offset : -offset));
    }
}

void SoulEngine::onAIResponseReceived(RobotEmotion e, int intensity, const String& text) {
    _state = STATE_AI_SPEAKING;
    triggerEmotion(e, intensity, 5000);
    _lastInteraction = millis();
    _disp->startScrollMessage(text, "PIKU AI");
    // Audio is handled by BrainEngine which calls audio->playPhonemes()
}

void SoulEngine::setAIThinking(bool thinking) {
    if (thinking) {
        _state  = STATE_AI_THINKING;
        _emotion = EMOTION_CURIOUS_SCAN;
        _disp->morphToEmotion(EMOTION_CURIOUS_SCAN, 200);
        _audio->playChirp(700, 1300, 160);
    } else {
        _state  = STATE_AWAKE_IDLE;
    }
}

void SoulEngine::onTouchShort() {
    _affection = min(100, _affection + 5);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_LOVE, 70, 3500);
    _audio->playHD(voice_love_data, sizeof(voice_love_data), 3, "I Love You! <3");
}

void SoulEngine::onTouchSustained() {
    _affection = min(100, _affection + 1);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_KAWAII_CAT, 55, 3000);
    _audio->playHD(voice_cat_data, sizeof(voice_cat_data), 4, "Nya! Meow!");
}

void SoulEngine::onTouchOverpet() {
    triggerEmotion(EMOTION_HYPNO_DIZZY, 80, 3000);
    _audio->playHD(voice_dizzy_data, sizeof(voice_dizzy_data), 1, "Dizzy! @__@");
}

void SoulEngine::onDoubleClap() {
    if (millis() < _clapCoolUntil) return;
    _clapCoolUntil = millis() + 5000;
    _lastInteraction = millis();
    triggerEmotion(EMOTION_PARTY_DJ, 100, 6000);
    _audio->playHD(voice_party_data, sizeof(voice_party_data), 1, "PARTY TIME!");
}
```

- [ ] **Step 3: Verify compiles**

Build with `pio run`. Expected: clean.

- [ ] **Step 4: Commit**

```bash
git add Piku/SoulEngine.h Piku/SoulEngine.cpp
git commit -m "feat(piku2): SoulEngine autonomous behaviour scheduler + emotional state machine"
```

---

## Task 6: BrainEngine — Gemini AI + NVS Owner Memory

**Files:**
- Create: `Piku/BrainEngine.h`
- Create: `Piku/BrainEngine.cpp`

**Interfaces:**
- Consumes: `SoulEngine::hasPendingAIRequest()`, `SoulEngine::consumeAIRequest()`, `SoulEngine::setAIThinking()`, `SoulEngine::onAIResponseReceived()`, `SoulEngine::getAffection/Energy/Hunger()`, all weather/time globals from NetworkEngine
- Produces: `BrainEngine::init()`, `BrainEngine::askGemini(const String& userPrompt)`, `BrainEngine::update()` (call from Core 0 task loop), `BrainEngine::setOwnerName(const String& name)`, `BrainEngine::getOwnerName()`, `BrainEngine::isOnboardingDone()`, `BrainEngine::getKeyCount()`, `BrainEngine::saveKeyPool(const String& keys)`, `BrainEngine::getLastReply()`

- [ ] **Step 1: Create BrainEngine.h**

```cpp
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <vector>
#include "SoulEngine.h"
#include "AudioEngine.h"

struct ContextTurn { String user, piku; };

class BrainEngine {
public:
    void init(SoulEngine* soul, AudioEngine* audio);
    void update();    // call from Core 0 task every loop

    // Gemini AI
    String askGemini(const String& userPrompt);
    String getLastReply() const { return _lastReply; }

    // Owner profile
    void   setOwnerName(const String& name);
    String getOwnerName()       const { return _ownerName; }
    bool   isOnboardingDone()   const { return _onboardingDone; }
    void   completeOnboarding();

    // Gemini key pool
    void saveKeyPool(const String& rawKeys);
    int  getKeyCount()          const { return (int)_keyPool.size(); }
    int  getActiveKeyIndex()    const { return _activeKey; }

    // Weather/time injection (set by NetworkEngine)
    int    currentTempC    = 25;
    int    currentHumidity = 65;
    String currentWeather  = "Sunny";
    String currentTime     = "12:00";
    String currentDate     = "Thu, 01 Jan";

private:
    SoulEngine*   _soul;
    AudioEngine*  _audio;
    Preferences   _prefs;

    String _ownerName      = "Friend";
    bool   _onboardingDone = false;
    char   _knownFacts[512];

    std::vector<String> _keyPool;
    int _activeKey = 0;

    ContextTurn _context[6];
    int _contextCount = 0;

    String _lastReply;

    void   _loadProfile();
    void   _saveProfile();
    void   _initKeyPool();
    String _buildPrompt(const String& userMessage, bool isAutonomous);
    void   _parseAndAct(const String& aiText, const String& userMessage, bool isAutonomous);
    void   _pushContext(const String& user, const String& piku);
    String _extractJsonString(const String& json, const String& key);
};
```

- [ ] **Step 2: Create BrainEngine.cpp**

```cpp
#include "BrainEngine.h"
#include "secrets.h"

void BrainEngine::init(SoulEngine* soul, AudioEngine* audio) {
    _soul  = soul;
    _audio = audio;
    memset(_knownFacts, 0, sizeof(_knownFacts));
    _loadProfile();
    _initKeyPool();
}

void BrainEngine::update() {
    if (!_soul->hasPendingAIRequest()) return;
    String topic = _soul->consumeAIRequest();
    askGemini(topic);   // autonomous call
}

void BrainEngine::_loadProfile() {
    _prefs.begin("piku", true);
    String name = _prefs.getString("owner_name", "");
    if (name.length() > 0) {
        _ownerName = name;
    }
    _onboardingDone = _prefs.getBool("onboarding", false);
    String facts    = _prefs.getString("owner_facts", "");
    if (facts.length() > 0) {
        strncpy(_knownFacts, facts.c_str(), 511);
    }
    _prefs.end();
}

void BrainEngine::_saveProfile() {
    _prefs.begin("piku", false);
    _prefs.putString("owner_name", _ownerName);
    _prefs.putBool("onboarding", _onboardingDone);
    _prefs.putString("owner_facts", String(_knownFacts));
    _prefs.end();
}

void BrainEngine::setOwnerName(const String& name) {
    _ownerName = name;
    _saveProfile();
}

void BrainEngine::completeOnboarding() {
    _onboardingDone = true;
    _saveProfile();
}

void BrainEngine::saveKeyPool(const String& rawKeys) {
    _prefs.begin("piku", false);
    _prefs.putString("gem_keys", rawKeys);
    _prefs.end();
    _initKeyPool();
}

void BrainEngine::_initKeyPool() {
    _keyPool.clear();
    _activeKey = 0;

    _prefs.begin("piku", true);
    String poolStr = _prefs.getString("gem_keys", "");
    _prefs.end();

    if (poolStr.length() > 10) {
        int start = 0;
        while (start < (int)poolStr.length()) {
            int comma = poolStr.indexOf(',', start);
            if (comma == -1) comma = poolStr.length();
            String k = poolStr.substring(start, comma);
            k.trim();
            if (k.length() > 10) _keyPool.push_back(k);
            start = comma + 1;
        }
    }

    const char* defaults[] = {
        DEFAULT_GEMINI_API_KEY, DEFAULT_GEMINI_KEY_2,
        DEFAULT_GEMINI_KEY_3,   DEFAULT_GEMINI_KEY_4, DEFAULT_GEMINI_KEY_5
    };
    for (int i = 0; i < 5; i++) {
        String dk = String(defaults[i]);
        dk.trim();
        if (dk.length() > 10 && dk != "YOUR_GEMINI_API_KEY_1_HERE") {
            bool exists = false;
            for (const auto& k : _keyPool) if (k == dk) { exists = true; break; }
            if (!exists) _keyPool.push_back(dk);
        }
    }
    Serial.printf("[Brain] Loaded %d Gemini keys\n", (int)_keyPool.size());
}

void BrainEngine::_pushContext(const String& user, const String& piku) {
    if (_contextCount < 6) {
        _context[_contextCount++] = {user, piku};
    } else {
        for (int i = 0; i < 5; i++) _context[i] = _context[i+1];
        _context[5] = {user, piku};
    }
}

String BrainEngine::_buildPrompt(const String& userMessage, bool isAutonomous) {
    String p = F("You are PIKU, a living AI desk companion robot. Personality: playful, witty, affectionate, curious, slightly mischievous.\n");
    p += "Owner: " + _ownerName + ".\n";
    if (strlen(_knownFacts) > 2) {
        p += "Known facts about owner: " + String(_knownFacts) + "\n";
    }
    p += "State: Hunger=" + String(_soul->getHunger()) + "%, Energy=" + String(_soul->getEnergy()) + "%, Affection=" + String(_soul->getAffection()) + "%.\n";
    p += "Time: " + currentTime + ". Date: " + currentDate + ". Weather: " + String(currentTempC) + "C " + currentWeather + ".\n";
    p += F("Rules:\n");
    p += F("1. Start with EXACTLY ONE tag: [HAPPY:n], [LOVE:n], [CURIOUS:n], [PARTY:n], [CAT:n], [COOL:n], [HACKER:n], [KISS:n], [ANGER:n], [SLEEPY:n], [SAD:n] — n=0-100 intensity.\n");
    p += F("2. Max 18 words after the tag. Natural, alive, punchy.\n");
    p += F("3. If you learn something important about the owner, append [REMEMBER: one sentence fact].\n");
    if (_contextCount > 0) {
        p += F("Recent dialogue:\n");
        for (int i = 0; i < _contextCount; i++) {
            p += _ownerName + ": " + _context[i].user + "\nPiku: " + _context[i].piku + "\n";
        }
    }
    if (isAutonomous) p += F("(Speak spontaneously — no prompt from owner.) ");
    p += "Message: " + userMessage;
    return p;
}

String BrainEngine::_extractJsonString(const String& json, const String& key) {
    String search = "\"" + key + "\": \"";
    int idx = json.indexOf(search);
    if (idx == -1) { search = "\"" + key + "\":\""; idx = json.indexOf(search); }
    if (idx == -1) return "";
    int start = idx + search.length();
    // Find end — skip escaped quotes
    int end = start;
    while (end < (int)json.length()) {
        if (json[end] == '\\') { end += 2; continue; }
        if (json[end] == '"') break;
        end++;
    }
    return json.substring(start, end);
}

String BrainEngine::askGemini(const String& userPrompt) {
    if (_keyPool.empty()) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 70, 3000);
        _audio->playHD(voice_uhoh_data, sizeof(voice_uhoh_data), 0, "No API key!");
        return "Please add a Gemini API key in Settings!";
    }
    if (WiFi.status() != WL_CONNECTED) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 60, 3000);
        return "No internet — connect Piku to WiFi first!";
    }

    bool isAuto = (userPrompt.indexOf("Comment on") == 0 ||
                   userPrompt.indexOf("Say something") == 0 ||
                   userPrompt.indexOf("Ask ") == 0 ||
                   userPrompt.indexOf("Share ") == 0);

    _soul->setAIThinking(true);

    String prompt = _buildPrompt(userPrompt, isAuto);
    prompt.replace("\"", "\\\"");
    prompt.replace("\n", "\\n");
    prompt.replace("\r", "");

    String payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + prompt + "\"}]}]}";

    const char* endpoints[] = {
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=",
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-lite-latest:generateContent?key="
    };

    String aiText;
    int totalKeys = (int)_keyPool.size();
    int attempts  = 0;

    while (attempts < totalKeys && aiText.length() == 0) {
        String key = _keyPool[_activeKey];
        for (int m = 0; m < 2 && aiText.length() == 0; m++) {
            WiFiClientSecure client;
            client.setInsecure();
            client.setTimeout(8000);
            HTTPClient https;
            String url = String(endpoints[m]) + key;
            if (https.begin(client, url)) {
                https.addHeader("Content-Type", "application/json");
                int code = https.POST(payload);
                if (code == 200) {
                    String resp = https.getString();
                    int ti = resp.indexOf("\"text\": \"");
                    if (ti == -1) ti = resp.indexOf("\"text\":\"");
                    if (ti != -1) {
                        String sub = resp.substring(ti + 9);
                        // Scan for unescaped closing quote
                        int end = 0;
                        while (end < (int)sub.length()) {
                            if (sub[end] == '\\') { end += 2; continue; }
                            if (sub[end] == '"') break;
                            end++;
                        }
                        aiText = sub.substring(0, end);
                        aiText.replace("\\n", " ");
                        aiText.replace("\\\"", "\"");
                    }
                } else if (code == 429 || code == 403 || code == 503) {
                    _activeKey = (_activeKey + 1) % totalKeys;
                }
                https.end();
            }
        }
        if (aiText.length() == 0) { _activeKey = (_activeKey + 1) % totalKeys; attempts++; }
    }

    _soul->setAIThinking(false);

    if (aiText.length() == 0) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 60, 3000);
        _audio->playHD(voice_uhoh_data, sizeof(voice_uhoh_data), 0, "Keys busy...");
        return "All Gemini keys are rate-limited. Try again soon!";
    }

    _lastReply = aiText;
    _parseAndAct(aiText, userPrompt, isAuto);
    return aiText;
}

void BrainEngine::_parseAndAct(const String& aiText, const String& userMessage, bool isAuto) {
    // Parse emotion tag + intensity: [EMOTION:n]
    RobotEmotion emotion = EMOTION_HELLO;
    int intensity = 65;

    struct { const char* tag; RobotEmotion e; } tagMap[] = {
        {"[HAPPY", EMOTION_HELLO}, {"[LOVE", EMOTION_LOVE}, {"[CURIOUS", EMOTION_CURIOUS_SCAN},
        {"[PARTY", EMOTION_PARTY_DJ}, {"[CAT", EMOTION_KAWAII_CAT}, {"[COOL", EMOTION_COOL_SUNGLASSES},
        {"[HACKER", EMOTION_MATRIX_HACKER}, {"[KISS", EMOTION_KAWAII_KISS}, {"[ANGER", EMOTION_FIRE_RAGE},
        {"[SLEEPY", EMOTION_SLEEP}, {"[SAD", EMOTION_RAINY_SAD}
    };
    for (auto& t : tagMap) {
        int idx = aiText.indexOf(t.tag);
        if (idx != -1) {
            emotion = t.e;
            int colon = aiText.indexOf(':', idx);
            int bracket = aiText.indexOf(']', idx);
            if (colon != -1 && colon < bracket) {
                intensity = aiText.substring(colon + 1, bracket).toInt();
                intensity = constrain(intensity, 0, 100);
            }
            break;
        }
    }

    // Extract clean text (after ']')
    int closeBracket = aiText.indexOf(']');
    String cleanText = (closeBracket != -1) ? aiText.substring(closeBracket + 1) : aiText;
    cleanText.trim();

    // Check for [REMEMBER: ...] and store
    int remIdx = cleanText.indexOf("[REMEMBER:");
    if (remIdx != -1) {
        int remEnd = cleanText.indexOf(']', remIdx);
        if (remEnd != -1) {
            String fact = cleanText.substring(remIdx + 10, remEnd);
            fact.trim();
            // Append to knownFacts with separator
            int curLen = strlen(_knownFacts);
            if (curLen < 480) {
                strncat(_knownFacts, " | ", 511 - curLen);
                strncat(_knownFacts, fact.c_str(), 511 - strlen(_knownFacts));
                _saveProfile();
            }
            // Remove [REMEMBER:...] from displayed text
            cleanText = cleanText.substring(0, remIdx) + cleanText.substring(remEnd + 1);
            cleanText.trim();
        }
    }

    // Count words for phoneme length
    int words = 1;
    for (int i = 0; i < (int)cleanText.length(); i++) if (cleanText[i] == ' ') words++;

    // Update context
    _pushContext(isAuto ? "(spontaneous)" : userMessage, cleanText);

    // Tell SoulEngine
    _soul->onAIResponseReceived(emotion, intensity, cleanText);

    // Play phoneme voice synced with text
    _audio->playPhonemes(words, cleanText.c_str());
}
```

- [ ] **Step 3: Verify compiles**

Build with `pio run`. Expected: clean.

- [ ] **Step 4: Commit**

```bash
git add Piku/BrainEngine.h Piku/BrainEngine.cpp
git commit -m "feat(piku2): BrainEngine with Gemini AI, owner memory, context, [REMEMBER] extraction"
```

---

## Task 7: NetworkEngine — WiFi + NTP + Weather

**Files:**
- Create: `Piku/NetworkEngine.h`
- Create: `Piku/NetworkEngine.cpp`

**Interfaces:**
- Produces: `NetworkEngine::init()`, `NetworkEngine::connectStation(const String& ssid, const String& pass)`, `NetworkEngine::syncNTP(int gmtOffsetHours)`, `NetworkEngine::fetchWeather(float lat, float lon)`, `NetworkEngine::getFormattedTime()`, `NetworkEngine::getFormattedDate()`, `NetworkEngine::isStaConnected()`, `NetworkEngine::getStaIP()`, `NetworkEngine::scanNetworks()`
- Weather results written into `BrainEngine` pointers: `currentTempC`, `currentHumidity`, `currentWeather`

- [ ] **Step 1: Create NetworkEngine.h**

```cpp
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <time.h>
#include <HTTPClient.h>
#include "config.h"

class NetworkEngine {
public:
    void init(const char* apSsid, const char* apPass, const char* mdnsName);
    void connectStation(const String& ssid, const String& pass, int gmtOffsetHours);
    void saveStaCreds(const String& ssid, const String& pass);
    bool isStaConnected() const { return WiFi.status() == WL_CONNECTED; }
    String getStaIP()     const { return WiFi.localIP().toString(); }
    String getApIP()      const { return WiFi.softAPIP().toString(); }

    void syncNTP(int gmtOffsetHours);
    void fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition);

    String getFormattedTime();
    String getFormattedDate();

    String scanNetworks();   // returns JSON array string

    bool timeIsSynced    = false;
    bool weatherIsSynced = false;

private:
    int _gmtOffset = 6;
};
```

- [ ] **Step 2: Create NetworkEngine.cpp**

```cpp
#include "NetworkEngine.h"

void NetworkEngine::init(const char* apSsid, const char* apPass, const char* mdnsName) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid, apPass);
    Serial.printf("[WiFi] AP: %s  IP: %s\n", apSsid, WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(mdnsName)) {
        Serial.printf("[mDNS] http://%s.local\n", mdnsName);
    }
}

void NetworkEngine::connectStation(const String& ssid, const String& pass, int gmtOffsetHours) {
    _gmtOffset = gmtOffsetHours;
    WiFi.begin(ssid.c_str(), pass.c_str());
}

void NetworkEngine::saveStaCreds(const String& ssid, const String& pass) {
    Preferences p; p.begin("piku", false);
    p.putString("sta_ssid", ssid);
    p.putString("sta_pass", pass);
    p.end();
}

void NetworkEngine::syncNTP(int gmtOffsetHours) {
    _gmtOffset = gmtOffsetHours;
    configTime(gmtOffsetHours * 3600, 0, NTP_SERVER, "time.nist.gov", "time.google.com");
    struct tm t; timeIsSynced = getLocalTime(&t);
    if (timeIsSynced) Serial.println(F("[NTP] Time synced"));
}

void NetworkEngine::fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition) {
    if (WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) +
                 "&current_weather=true&hourly=relativehumidity_2m&forecast_days=1&timezone=auto";
    http.begin(url);
    if (http.GET() == 200) {
        String body = http.getString();
        int ti = body.indexOf("\"temperature\":");
        if (ti != -1) *outTemp = (int)body.substring(ti + 14, body.indexOf(',', ti)).toFloat();
        int wi = body.indexOf("\"weathercode\":");
        int code = (wi != -1) ? body.substring(wi + 14, body.indexOf(',', wi)).toInt() : 0;
        if      (code == 0)          *outCondition = "Sunny";
        else if (code <= 3)          *outCondition = "Partly Cloudy";
        else if (code <= 48)         *outCondition = "Foggy";
        else if (code <= 67)         *outCondition = "Rainy";
        else if (code <= 77)         *outCondition = "Snowy";
        else                         *outCondition = "Stormy";
        int hi = body.indexOf("\"relativehumidity_2m\":[");
        if (hi != -1) {
            int start = hi + 23;
            *outHumidity = body.substring(start, body.indexOf(',', start)).toInt();
        }
        weatherIsSynced = true;
        Serial.printf("[Weather] %d°C %s\n", *outTemp, outCondition->c_str());
    }
    http.end();
}

String NetworkEngine::getFormattedTime() {
    struct tm t;
    if (!getLocalTime(&t)) return "--:--";
    char buf[10];
    strftime(buf, sizeof(buf), "%I:%M %p", &t);
    return String(buf);
}

String NetworkEngine::getFormattedDate() {
    struct tm t;
    if (!getLocalTime(&t)) return "---";
    char buf[22];
    strftime(buf, sizeof(buf), "%a, %d %b %Y", &t);
    return String(buf);
}

String NetworkEngine::scanNetworks() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    return json;
}
```

- [ ] **Step 3: Verify compiles**

Build with `pio run`. Expected: clean.

- [ ] **Step 4: Commit**

```bash
git add Piku/NetworkEngine.h Piku/NetworkEngine.cpp
git commit -m "feat(piku2): NetworkEngine WiFi AP+STA, NTP, Open-Meteo weather, mDNS"
```

---

## Task 8: WebUI.h — Lightweight Minimal Web Dashboard

**Files:**
- Create: `Piku/WebUI.h`

This file contains the entire web dashboard as a `PROGMEM const char[]` string. The HTML/CSS/JS is designed to be < 12 KB total.

**Design system:** Dark bg `#111`, text `#eee`, accent `#00d4aa`, secondary `#888`. Clean monospace touches. No blur, no heavy CSS. Fast first paint.

**4 Tabs:** Status, Chat, Controls, Settings.

- [ ] **Step 1: Create WebUI.h with full HTML**

```cpp
#pragma once
#include <Arduino.h>

const char WEBUI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>PIKU AI</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#111;color:#eee;font-family:system-ui,-apple-system,sans-serif;padding:10px;min-height:100vh}
:root{--a:#00d4aa;--b:#111;--c:#1a1a1a;--d:#2a2a2a;--e:#888}
.wrap{max-width:480px;margin:0 auto;padding-bottom:24px}
h1{font-size:18px;color:var(--a);letter-spacing:.5px;text-align:center;padding:10px 0 6px}
.pill{display:inline-block;font-size:11px;padding:3px 10px;border-radius:20px;border:1px solid var(--e);color:var(--e)}
.pill.on{border-color:var(--a);color:var(--a)}
.status-row{display:flex;justify-content:space-between;align-items:center;background:var(--c);border-radius:10px;padding:10px 12px;margin-bottom:10px;font-size:13px}
.tabs{display:flex;gap:4px;margin-bottom:10px}
.tab{flex:1;background:var(--c);border:none;color:var(--e);padding:8px 4px;font-size:12px;border-radius:8px;cursor:pointer}
.tab.active{background:var(--a);color:#111;font-weight:700}
.pane{display:none}.pane.active{display:block}
.card{background:var(--c);border-radius:10px;padding:12px;margin-bottom:10px}
.label{font-size:11px;color:var(--e);margin-bottom:4px}
.vbar{background:var(--d);border-radius:4px;height:7px;margin-top:4px;overflow:hidden}
.vfill{height:100%;background:var(--a);transition:width .4s}
.vrow{display:flex;justify-content:space-between;font-size:12px;margin-bottom:6px}
.chat{height:150px;overflow-y:auto;background:var(--d);border-radius:8px;padding:8px;margin-bottom:8px;font-size:12px;display:flex;flex-direction:column;gap:6px}
.bu{padding:7px 10px;border-radius:10px;max-width:85%;line-height:1.4}
.bu.u{background:#004433;align-self:flex-end}
.bu.a{background:#1a1a2e;align-self:flex-start}
.row{display:flex;gap:6px}
input,textarea,select{flex:1;background:var(--d);border:1px solid #333;color:#eee;padding:9px 10px;border-radius:8px;font-size:12px;outline:none;width:100%}
input:focus,textarea:focus,select:focus{border-color:var(--a)}
.btn{background:var(--d);border:1px solid #333;color:#eee;padding:9px 10px;border-radius:8px;font-size:12px;cursor:pointer;display:flex;flex-direction:column;align-items:center;gap:3px;text-align:center}
.btn:active{background:var(--a);color:#111;border-color:var(--a)}
.btn.p{background:var(--a);color:#111;border:none;font-weight:700}
.btn.s{background:#4a1540;border-color:#993388;color:#ee88cc}
.g4{display:grid;grid-template-columns:repeat(4,1fr);gap:6px}
.g3{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}
.slider{width:100%;accent-color:var(--a);margin:8px 0}
.toggle{display:flex;justify-content:space-between;align-items:center;font-size:12px;padding:6px 0}
.sw{position:relative;display:inline-block;width:36px;height:20px}
.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;cursor:pointer;inset:0;background:#333;border-radius:20px;transition:.3s}
.sw span:before{position:absolute;content:"";width:14px;height:14px;left:3px;top:3px;background:#eee;border-radius:50%;transition:.3s}
.sw input:checked+span{background:var(--a)}
.sw input:checked+span:before{transform:translateX(16px)}
.ai-resp{font-size:12px;color:#ccc;background:var(--d);border-radius:8px;padding:10px;min-height:40px;margin-bottom:8px;line-height:1.5}
.chips{display:flex;gap:4px;flex-wrap:wrap;margin-top:6px}
.chip{font-size:10px;padding:4px 8px;background:var(--d);border:1px solid #333;border-radius:16px;cursor:pointer;white-space:nowrap}
.chip:active{background:var(--a);color:#111;border-color:var(--a)}
.emo-ico{font-size:18px}
</style>
</head>
<body>
<div class="wrap">
<h1>🤖 PIKU</h1>
<div class="status-row">
  <div>
    <span id="cpill" class="pill">AP 192.168.4.1</span>
    <div id="owner-name" style="font-size:11px;color:#888;margin-top:4px"></div>
  </div>
  <div style="text-align:right;font-size:12px">
    <div id="clk" style="color:var(--a);font-weight:700">--:--</div>
    <div id="wth" style="color:#888;font-size:11px">--°C</div>
  </div>
</div>

<div class="tabs">
  <button class="tab active" onclick="tab(0)">Status</button>
  <button class="tab" onclick="tab(1)">Chat</button>
  <button class="tab" onclick="tab(2)">Controls</button>
  <button class="tab" onclick="tab(3)">Settings</button>
</div>

<!-- STATUS TAB -->
<div class="pane active" id="p0">
  <div class="card">
    <div class="label">VITALS</div>
    <div class="vrow"><span>❤️ Affection</span><span id="sa">85%</span></div>
    <div class="vbar"><div class="vfill" id="ba" style="width:85%"></div></div>
    <div class="vrow" style="margin-top:8px"><span>⚡ Energy</span><span id="se">100%</span></div>
    <div class="vbar"><div class="vfill" id="be" style="width:100%;background:#fbbf24"></div></div>
    <div class="vrow" style="margin-top:8px"><span>🍕 Hunger</span><span id="sh">90%</span></div>
    <div class="vbar"><div class="vfill" id="bh" style="width:90%;background:#f97316"></div></div>
  </div>
  <div class="card">
    <div class="label">LAST AI RESPONSE</div>
    <div class="ai-resp" id="last-resp">Hi! I am Piku. Ask me anything!</div>
  </div>
  <div class="card">
    <div class="label">SPONTANEOUS TALK INTERVAL</div>
    <select id="auto-talk-sel" onchange="setAutoTalk(this.value)" style="margin-top:6px">
      <option value="0">Off</option>
      <option value="5">Every 5 minutes</option>
      <option value="10" selected>Every 10 minutes</option>
      <option value="20">Every 20 minutes</option>
      <option value="30">Every 30 minutes</option>
    </select>
  </div>
</div>

<!-- CHAT TAB -->
<div class="pane" id="p1">
  <div class="card">
    <div class="label" style="display:flex;justify-content:space-between">
      <span>CHAT WITH PIKU</span>
      <span id="ai-badge" style="font-size:11px;color:#888">● Ready</span>
    </div>
    <div class="chat" id="clog">
      <div class="bu a">🤖 Hi! I'm Piku! Ask me anything or tap a quick question below.</div>
    </div>
    <div class="row">
      <input type="text" id="ainp" placeholder="Ask Piku..." onkeydown="if(event.key==='Enter')sendAI()">
      <button class="btn" id="mic-btn" onclick="toggleMic()" style="min-width:40px;padding:9px">🎙️</button>
      <button class="btn p" onclick="sendAI()" style="padding:9px 14px">Ask</button>
    </div>
    <div class="chips">
      <div class="chip" onclick="chip('Who are you?')">🤖 Who are you?</div>
      <div class="chip" onclick="chip('What is the weather like?')">🌤️ Weather?</div>
      <div class="chip" onclick="chip('Tell me a short witty joke!')">😂 Joke</div>
      <div class="chip" onclick="chip('How are you feeling right now?')">❤️ Feeling?</div>
    </div>
    <div class="toggle" style="margin-top:8px;border-top:1px solid #222;padding-top:8px">
      <span style="font-size:11px;color:#888">🔊 Speak replies in browser</span>
      <label class="sw"><input type="checkbox" id="tts-tog" checked><span></span></label>
    </div>
  </div>
</div>

<!-- CONTROLS TAB -->
<div class="pane" id="p2">
  <div class="card">
    <div class="label" style="display:flex;justify-content:space-between">
      <span>VOLUME</span><span id="vol-lbl">80%</span>
    </div>
    <input type="range" min="0" max="100" value="80" class="slider" id="vol-sl" oninput="setVol(this.value)">
    <div class="g4" style="margin-top:4px">
      <button class="btn" onclick="setVol(0)">🔇</button>
      <button class="btn" onclick="setVol(40)">40%</button>
      <button class="btn" onclick="setVol(80)">80%</button>
      <button class="btn" onclick="setVol(100)">📢</button>
    </div>
  </div>
  <div class="card">
    <div class="label">EXPRESSIONS</div>
    <div class="g4" style="margin-top:6px">
      <button class="btn" onclick="cmd('hello')"><div class="emo-ico">👋</div><div>Hello</div></button>
      <button class="btn" onclick="cmd('love')"><div class="emo-ico">❤️</div><div>Love</div></button>
      <button class="btn" onclick="cmd('party')"><div class="emo-ico">🎉</div><div>Party</div></button>
      <button class="btn" onclick="cmd('shades')"><div class="emo-ico">😎</div><div>Cool</div></button>
      <button class="btn" onclick="cmd('cat')"><div class="emo-ico">🐱</div><div>Cat</div></button>
      <button class="btn" onclick="cmd('kiss')"><div class="emo-ico">😘</div><div>Kiss</div></button>
      <button class="btn" onclick="cmd('fire')"><div class="emo-ico">🔥</div><div>Fire</div></button>
      <button class="btn" onclick="cmd('matrix')"><div class="emo-ico">💻</div><div>Hacker</div></button>
      <button class="btn" onclick="cmd('clock')"><div class="emo-ico">🕒</div><div>Clock</div></button>
      <button class="btn" onclick="cmd('weather')"><div class="emo-ico">🌤️</div><div>Weather</div></button>
      <button class="btn" onclick="cmd('study')"><div class="emo-ico">👓</div><div>Study</div></button>
      <button class="btn" onclick="cmd('sleep')"><div class="emo-ico">💤</div><div>Sleep</div></button>
    </div>
  </div>
  <div class="card">
    <div class="label">MINI-GAMES</div>
    <div class="g3" style="margin-top:6px">
      <button class="btn p" onclick="cmd('flap_start')"><div class="emo-ico">🐤</div><div>Flappy</div></button>
      <button class="btn" onclick="cmd('flap_jump')"><div class="emo-ico">⬆️</div><div>Jump</div></button>
      <button class="btn" onclick="cmd('rps')"><div class="emo-ico">✂️</div><div>RPS</div></button>
      <button class="btn" onclick="cmd('8ball')"><div class="emo-ico">🎱</div><div>8-Ball</div></button>
      <button class="btn" onclick="cmd('snack')"><div class="emo-ico">🍕</div><div>Feed</div></button>
      <button class="btn s" onclick="cmd('sentry')"><div class="emo-ico">🚨</div><div>Sentry</div></button>
    </div>
  </div>
  <div class="card">
    <div class="label" style="display:flex;justify-content:space-between"><span>HEAD SERVO</span><span id="srv-lbl">90°</span></div>
    <input type="range" min="40" max="140" value="90" class="slider" id="srv-sl" oninput="steer(this.value)">
    <div class="g3" style="margin-top:4px">
      <button class="btn" onclick="steer(60)">◀ Left</button>
      <button class="btn" onclick="steer(90)">Center</button>
      <button class="btn" onclick="steer(120)">Right ▶</button>
    </div>
  </div>
  <div class="card">
    <div class="label">OLED BILLBOARD</div>
    <div class="row" style="margin-top:6px">
      <input type="text" id="bill-inp" placeholder="Message to scroll on OLED...">
      <button class="btn p" onclick="sendBill()" style="padding:9px 14px">Send</button>
    </div>
  </div>
</div>

<!-- SETTINGS TAB -->
<div class="pane" id="p3">
  <div class="card">
    <div class="label">OWNER PROFILE</div>
    <div style="font-size:12px;color:#888;margin:6px 0">Name: <span id="own-name-lbl" style="color:#eee">--</span></div>
    <button class="btn" onclick="resetOwner()" style="font-size:11px;padding:6px 12px;margin-top:4px">Reset Profile (clear memory)</button>
  </div>
  <div class="card">
    <div class="label">GEMINI API KEYS <span id="key-badge" style="color:#888;font-size:11px">(0 active)</span></div>
    <textarea id="key-inp" rows="3" placeholder="AIzaSy...&#10;AIzaSy... (one per line)" style="margin-top:6px"></textarea>
    <button class="btn p" onclick="saveKeys()" style="width:100%;margin-top:8px">Save Key Pool</button>
  </div>
  <div class="card">
    <div class="label">SOUND SENSOR (CLAP)</div>
    <div class="toggle">
      <span style="color:#888;font-size:12px">Enable double-clap DJ mode</span>
      <label class="sw"><input type="checkbox" id="mic-tog" onchange="setMic(this.checked)"><span></span></label>
    </div>
    <div style="font-size:10px;color:#555;margin-top:4px">Keep off in noisy environments.</div>
  </div>
  <div class="card">
    <div class="label">TIMEZONE & LOCATION</div>
    <select id="tz-sel" style="margin:6px 0">
      <option value="-8">UTC-8 (California)</option>
      <option value="-5">UTC-5 (New York)</option>
      <option value="0">UTC+0 (London)</option>
      <option value="1">UTC+1 (Paris)</option>
      <option value="5.5">UTC+5:30 (India)</option>
      <option value="6" selected>UTC+6 (Dhaka)</option>
      <option value="8">UTC+8 (Singapore)</option>
      <option value="9">UTC+9 (Tokyo)</option>
    </select>
    <div class="row" style="margin-bottom:8px">
      <input type="text" id="lat-inp" value="23.8103" placeholder="Latitude">
      <input type="text" id="lon-inp" value="90.4125" placeholder="Longitude">
    </div>
    <button class="btn p" onclick="saveLoc()" style="width:100%">Save & Resync</button>
  </div>
  <div class="card">
    <div class="label">WIFI ROUTER</div>
    <button class="btn" onclick="scanWifi()" style="width:100%;margin-bottom:8px">🔍 Scan Networks</button>
    <select id="wifi-sel" style="margin-bottom:8px"><option value="">-- Scan first --</option></select>
    <input type="text" id="wifi-pass" placeholder="WiFi Password" style="margin-bottom:8px">
    <button class="btn p" onclick="saveWifi()" style="width:100%">Connect & Save</button>
  </div>
</div>
</div>

<script>
function tab(i){
  document.querySelectorAll('.tab').forEach((t,j)=>t.classList.toggle('active',i===j));
  document.querySelectorAll('.pane').forEach((p,j)=>p.classList.toggle('active',i===j));
}
function cmd(c){fetch('/api?cmd='+c);}
function steer(a){
  document.getElementById('srv-sl').value=a;
  document.getElementById('srv-lbl').textContent=a+'°';
  fetch('/api?steer='+a);
}
function setVol(v){
  document.getElementById('vol-sl').value=v;
  document.getElementById('vol-lbl').textContent=v+'%';
  fetch('/api?vol='+v);
}
function setMic(e){fetch('/api?mic_en='+(e?1:0));}
function setAutoTalk(m){fetch('/api?autotalk='+m);}

function speakBrowser(t){
  if(!('speechSynthesis' in window)) return;
  if(!document.getElementById('tts-tog').checked) return;
  speechSynthesis.cancel();
  const u=new SpeechSynthesisUtterance(t.replace(/\[[A-Z:0-9]+\]/g,'').trim());
  u.rate=1.05;u.pitch=1.3;
  speechSynthesis.speak(u);
}

function addChat(text,type){
  const c=document.getElementById('clog');
  const d=document.createElement('div');
  d.className='bu '+(type==='user'?'u':'a');
  d.textContent=(type==='user'?'👤 ':'')+text;
  c.appendChild(d);c.scrollTop=c.scrollHeight;
  return d;
}
function chip(q){document.getElementById('ainp').value=q;sendAI();}
function sendAI(){
  const inp=document.getElementById('ainp');
  const q=inp.value.trim();if(!q)return;
  addChat(q,'user');inp.value='';
  const thinking=addChat('🤖 Thinking...','ai');
  document.getElementById('ai-badge').textContent='● Thinking...';
  fetch('/api/gemini/ask',{method:'POST',headers:{'Content-Type':'text/plain'},body:q})
  .then(r=>r.text()).then(ans=>{
    thinking.textContent='🤖 '+ans;
    document.getElementById('ai-badge').textContent='● Ready';
    document.getElementById('last-resp').textContent=ans;
    document.getElementById('clog').scrollTop=99999;
    speakBrowser(ans);
  }).catch(()=>{
    thinking.textContent='⚠️ Could not reach Piku.';
    document.getElementById('ai-badge').textContent='● Offline';
  });
}

let recog=null;
function toggleMic(){
  if(!('webkitSpeechRecognition' in window)&&!('SpeechRecognition' in window)){
    alert('Voice input requires Chrome or Edge.');return;
  }
  const btn=document.getElementById('mic-btn');
  if(!recog){
    const SR=window.SpeechRecognition||window.webkitSpeechRecognition;
    recog=new SR();recog.continuous=false;recog.interimResults=false;
    recog.onstart=()=>btn.textContent='🔴';
    recog.onresult=e=>{document.getElementById('ainp').value=e.results[0][0].transcript;btn.textContent='🎙️';sendAI();};
    recog.onerror=recog.onend=()=>btn.textContent='🎙️';
  }
  recog.start();
}

function scanWifi(){
  const sel=document.getElementById('wifi-sel');
  sel.innerHTML='<option>Scanning...</option>';
  fetch('/api/wifi/scan',{method:'POST'}).then(r=>r.json()).then(nets=>{
    sel.innerHTML='';
    if(!nets.length){sel.innerHTML='<option>No networks found</option>';return;}
    nets.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+'dBm)';sel.appendChild(o);});
  }).catch(()=>sel.innerHTML='<option>Scan failed</option>');
}
function saveWifi(){
  const s=document.getElementById('wifi-sel').value;
  const p=document.getElementById('wifi-pass').value;
  if(!s){alert('Select a network first!');return;}
  fetch('/api/wifi/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:s,pass:p})})
  .then(()=>alert('Connecting to '+s+'...'));
}
function saveKeys(){
  const k=document.getElementById('key-inp').value.trim();
  if(!k){alert('Enter at least one key!');return;}
  fetch('/api/gemini/key',{method:'POST',body:k}).then(()=>alert('Key pool saved!'));
}
function saveLoc(){
  const tz=document.getElementById('tz-sel').value;
  const lat=document.getElementById('lat-inp').value;
  const lon=document.getElementById('lon-inp').value;
  fetch('/api/settings/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tz:parseFloat(tz),lat:parseFloat(lat),lon:parseFloat(lon)})})
  .then(()=>{alert('Saved! Resyncing...');fetch('/api/sync',{method:'POST'});});
}
function sendBill(){
  const m=document.getElementById('bill-inp').value.trim();
  if(!m)return;
  fetch('/api/billboard',{method:'POST',body:m}).then(()=>alert('Sent!'));
}
function resetOwner(){
  if(confirm('Clear owner profile and memory?'))
    fetch('/api/owner/reset',{method:'POST'}).then(()=>alert('Profile reset! Reboot Piku.'));
}

function poll(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    // Vitals
    ['aff','eng','hng'].forEach((k,i)=>{
      const vals=[d.affection,d.energy,d.hunger];
      document.getElementById('s'+k[0]).textContent=vals[i]+'%';
      document.getElementById('b'+k[0]).style.width=vals[i]+'%';
    });
    // Sliders (skip if focused)
    const vs=document.getElementById('vol-sl');
    if(document.activeElement!==vs){vs.value=d.volume;document.getElementById('vol-lbl').textContent=(d.is_muted?'Muted':d.volume+'%');}
    const ss=document.getElementById('srv-sl');
    if(document.activeElement!==ss){ss.value=d.servo_angle;document.getElementById('srv-lbl').textContent=d.servo_angle+'°';}
    // Clock + weather
    document.getElementById('clk').textContent=d.time_str;
    document.getElementById('wth').textContent=d.temp_c+'°C '+d.weather;
    // Connection pill
    const pill=document.getElementById('cpill');
    if(d.sta_connected){pill.textContent='🟢 '+d.sta_ip;pill.className='pill on';}
    else{pill.textContent='AP 192.168.4.1';pill.className='pill';}
    // Mic toggle
    const mt=document.getElementById('mic-tog');if(document.activeElement!==mt)mt.checked=d.mic_enabled;
    // Key badge
    document.getElementById('key-badge').textContent='('+d.key_count+' active)';
    // Owner name
    document.getElementById('own-name-lbl').textContent=d.owner_name||'--';
    document.getElementById('owner-name').textContent=d.owner_name?('Hi, '+d.owner_name+'!'):'';
    // Auto-talk sel
    const at=document.getElementById('auto-talk-sel');
    if(document.activeElement!==at&&d.auto_talk_min!==undefined)at.value=d.auto_talk_min;
    // Last AI resp
    if(d.last_reply&&d.last_reply.length>2)document.getElementById('last-resp').textContent=d.last_reply;
  }).catch(()=>{
    document.getElementById('cpill').textContent='🔴 Disconnected';
    document.getElementById('cpill').className='pill';
  });
}
setInterval(poll,2000);poll();
</script>
</body>
</html>
)rawliteral";
```

- [ ] **Step 2: Verify total page size**

```bash
python -c "
txt = open('Piku/WebUI.h','r',encoding='utf-8').read()
import re
m = re.search(r'R\"rawliteral\((.*?)\)rawliteral\"', txt, re.DOTALL)
if m: print(f'HTML size: {len(m.group(1).encode())} bytes = {len(m.group(1).encode())/1024:.1f} KB')
"
```
Expected: < 12 KB.

- [ ] **Step 3: Verify compiles**

Build with `pio run`. Expected: clean.

- [ ] **Step 4: Commit**

```bash
git add Piku/WebUI.h
git commit -m "feat(piku2): WebUI lightweight minimal dashboard <12KB, 4 tabs, clean dark theme"
```

---

## Task 9: Piku.ino — Main Entry + FreeRTOS Dual-Core Assembly

**Files:**
- Overwrite: `Piku/Piku.ino`

This is the final integration. All modules are instantiated here. Two FreeRTOS tasks are launched. The main `loop()` is left empty.

The Web Server and all HTTP handlers live in `Piku.ino` since they access multiple modules. The `handleStatus()` JSON is expanded to include `owner_name`, `auto_talk_min`, `last_reply`. All old game/expression/mini-game logic is delegated to module methods.

- [ ] **Step 1: Write the new Piku.ino**

Full code for `Piku.ino`:

```cpp
// =========================================================================
// 🤖 PIKU 2.0 — AUTONOMOUS AI DESK COMPANION
//    FreeRTOS Dual-Core | Modular C++ | Gemini AI | Owner Memory
// =========================================================================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <vector>

#include "config.h"
#include "secrets.h"
#include "voice_samples.h"
#include "ServoEngine.h"
#include "AudioEngine.h"
#include "SensorEngine.h"
#include "DisplayEngine.h"
#include "SoulEngine.h"
#include "BrainEngine.h"
#include "NetworkEngine.h"
#include "WebUI.h"

// ─── Module Instances ────────────────────────────────────────────────────────
ServoEngine  servo;
AudioEngine  audio;
SensorEngine sensors;
DisplayEngine disp;
SoulEngine   soul;
BrainEngine  brain;
NetworkEngine net;
WebServer    server(80);

// ─── Persistent Settings ─────────────────────────────────────────────────────
int   gmtOffsetHours = DEFAULT_GMT_OFFSET;
float userLat = 23.8103f, userLon = 90.4125f;
int   masterVolume = 80;
bool  soundEnabled = false;

// ─── Game State ──────────────────────────────────────────────────────────────
int   flappyBirdY  = 28;
float flappyVel    = 0.0f;
int   flappyScore  = 0;
int   flappyPipeX  = 120;
int   flappyPipeGapY = 24;
bool  flappyOver   = false;
unsigned long nextFlappyTick = 0;
int   flappyHiScore = 0;
bool  sentryActive = false;
bool  snackActive  = false;
unsigned long snackEnd = 0;
bool  rpsActive    = false;
unsigned long rpsEnd = 0;
const char* rpsChoice = "ROCK";
const char* magic8Ans  = "YES!";
bool  magic8Active = false;
unsigned long magic8End = 0;

// ─── WiFi Watchdog ───────────────────────────────────────────────────────────
bool  wifiWasConnected = false;
unsigned long nextWifiCheck = 0;
unsigned long nextWeatherCheck = 0;

// ─── FreeRTOS Task: Core 0 — Network / AI ────────────────────────────────────
void networkTask(void*) {
    for (;;) {
        server.handleClient();

        unsigned long now = millis();

        // WiFi watchdog
        if (now >= nextWifiCheck) {
            nextWifiCheck = now + 5000;
            bool connected = net.isStaConnected();
            if (connected && !wifiWasConnected) {
                wifiWasConnected = true;
                Serial.printf("[WiFi] Connected! IP: %s\n", net.getStaIP().c_str());
                net.syncNTP(gmtOffsetHours);
                int t=25,h=65; String cond="Sunny";
                net.fetchWeather(userLat, userLon, &t, &h, &cond);
                brain.currentTempC = t; brain.currentHumidity = h; brain.currentWeather = cond;
                disp.startScrollMessage("WiFi OK! Time+Weather synced.", "ONLINE");
                audio.playHD(voice_tada_data, sizeof(voice_tada_data), 3, "WiFi Connected!");
            } else if (!connected) {
                wifiWasConnected = false;
            }
        }

        // Weather resync every 15 minutes when connected
        if (net.isStaConnected() && now >= nextWeatherCheck) {
            nextWeatherCheck = now + 900000;
            int t=brain.currentTempC, h=brain.currentHumidity;
            String cond=brain.currentWeather;
            net.fetchWeather(userLat, userLon, &t, &h, &cond);
            brain.currentTempC = t; brain.currentHumidity = h; brain.currentWeather = cond;
        }

        // Update time strings in brain
        if (net.timeIsSynced) {
            brain.currentTime = net.getFormattedTime();
            brain.currentDate = net.getFormattedDate();
        }

        // Autonomous AI talk (SoulEngine decides when)
        brain.update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── FreeRTOS Task: Core 1 — Companion Soul Loop ────────────────────────────
void soulTask(void*) {
    for (;;) {
        soul.update();
        sensors.update();
        servo.update();
        disp.update(&audio);

        // Game states
        unsigned long now = millis();
        if (soul.getState() == STATE_GAME_FLAPPY) {
            if (now >= nextFlappyTick) {
                nextFlappyTick = now + 50;
                flappyVel += 1.5f;
                flappyBirdY = constrain((int)(flappyBirdY + flappyVel), 0, 63);
                flappyPipeX -= 3;
                if (flappyPipeX < -16) {
                    flappyPipeX = 128; flappyPipeGapY = random(10, 32);
                    if (!flappyOver) { flappyScore++; if (flappyScore > flappyHiScore) flappyHiScore = flappyScore; }
                }
                bool hitPipe = (flappyBirdY < flappyPipeGapY || flappyBirdY > flappyPipeGapY + 28)
                               && (24 >= flappyPipeX && 24 <= flappyPipeX + 16);
                if (flappyBirdY <= 0 || flappyBirdY >= 63 || hitPipe) flappyOver = true;
                disp.renderFlappyGame(flappyBirdY, flappyVel, flappyScore, flappyHiScore, flappyPipeX, flappyPipeGapY, flappyOver);
            }
        }
        if (snackActive && now >= snackEnd) { snackActive = false; soul.setState(STATE_AWAKE_IDLE); }
        if (rpsActive && now >= rpsEnd)     { rpsActive = false;   soul.setState(STATE_AWAKE_IDLE); }
        if (magic8Active && now >= magic8End){ magic8Active=false; soul.triggerEmotion(EMOTION_IDLE,50,100); }

        if (rpsActive)    disp.renderRPS(rpsChoice);
        if (snackActive)  disp.renderSnackEat((int)(now/150)%4);
        if (magic8Active) disp.renderMagic8Ball(magic8Ans);

        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

// ─── Web Server Handlers ──────────────────────────────────────────────────────
void handleRoot() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.send(200, "text/html", FPSTR(WEBUI_HTML));
}

void handleCommand() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    soul.feedSnack(); // any interaction boosts affection slightly

    if (server.hasArg("vol")) {
        int v = server.arg("vol").toInt();
        audio.setVolume(v);
        masterVolume = v;
        disp.showVolumeHUD(v, audio.isMuted());
        Preferences p; p.begin("piku",false); p.putInt("volume",v); p.end();
        server.send(200,"text/plain",String(v));
        return;
    }
    if (server.hasArg("steer")) {
        servo.setTarget(constrain(server.arg("steer").toInt(), 40, 140));
        server.send(200,"text/plain","OK"); return;
    }
    if (server.hasArg("mic_en")) {
        soundEnabled = server.arg("mic_en").toInt() == 1;
        sensors.setSoundEnabled(soundEnabled);
        Preferences p; p.begin("piku",false); p.putBool("mic_en",soundEnabled); p.end();
        server.send(200,"text/plain","OK"); return;
    }
    if (server.hasArg("autotalk")) {
        int m = server.arg("autotalk").toInt();
        soul.setAutoTalkInterval(m);
        Preferences p; p.begin("piku",false); p.putInt("auto_talk",m); p.end();
        server.send(200,"text/plain","OK"); return;
    }
    if (server.hasArg("cmd")) {
        String c = server.arg("cmd");
        if      (c=="hello")  { soul.triggerEmotion(EMOTION_HELLO,70,3000); audio.playHD(voice_hello_data,sizeof(voice_hello_data),2,"Hi! I am Piku!"); }
        else if (c=="love")   { soul.triggerEmotion(EMOTION_LOVE,80,4000); audio.playHD(voice_love_data,sizeof(voice_love_data),3,"I Love You!"); }
        else if (c=="party")  { soul.triggerEmotion(EMOTION_PARTY_DJ,100,5000); audio.playHD(voice_party_data,sizeof(voice_party_data),1,"PARTY TIME!"); }
        else if (c=="shades") { soul.triggerEmotion(EMOTION_COOL_SUNGLASSES,70,4500); }
        else if (c=="cat")    { soul.triggerEmotion(EMOTION_KAWAII_CAT,70,4500); audio.playHD(voice_cat_data,sizeof(voice_cat_data),4,"Nya! Meow!"); }
        else if (c=="kiss")   { soul.triggerEmotion(EMOTION_KAWAII_KISS,75,4500); audio.playHD(voice_kiss_data,sizeof(voice_kiss_data),4,"Mwah!"); }
        else if (c=="fire")   { soul.triggerEmotion(EMOTION_FIRE_RAGE,90,4500); audio.playHD(voice_fire_data,sizeof(voice_fire_data),3,"POWER!"); }
        else if (c=="matrix") { soul.triggerEmotion(EMOTION_MATRIX_HACKER,70,4500); audio.playHD(voice_hacker_data,sizeof(voice_hacker_data),2,"ACCESS GRANTED!"); }
        else if (c=="clock")  { soul.triggerEmotion(EMOTION_CLOCK_DISPLAY,50,6000); audio.playChirp(800,1400,120); }
        else if (c=="weather"){ soul.triggerEmotion(EMOTION_WEATHER_DISPLAY,50,6000); audio.playChirp(600,1000,150); }
        else if (c=="study")  { soul.triggerEmotion(EMOTION_FOCUS_STUDY,60,15000); audio.playHD(voice_focus_data,sizeof(voice_focus_data),2,"Focus!"); }
        else if (c=="sleep")  { soul.setState(STATE_DEEP_SLEEP); soul.triggerEmotion(EMOTION_SLEEP,70,0); audio.playHD(voice_sleep_data,sizeof(voice_sleep_data),0,"Zzz..."); }
        else if (c=="sentry") { soul.setState(STATE_SENTRY_GUARD); sentryActive=true; audio.playHD(voice_sentry_data,sizeof(voice_sentry_data),1,"INTRUDER!"); }
        else if (c=="flap_start") {
            soul.setState(STATE_GAME_FLAPPY); flappyBirdY=28; flappyVel=0; flappyScore=0;
            flappyPipeX=120; flappyPipeGapY=24; flappyOver=false; nextFlappyTick=millis();
            audio.playHD(voice_game_data,sizeof(voice_game_data),3,"Game On!");
        }
        else if (c=="flap_jump") {
            if (soul.getState()==STATE_GAME_FLAPPY) {
                if (flappyOver) { flappyBirdY=28;flappyVel=0;flappyScore=0;flappyPipeX=120;flappyOver=false; }
                else flappyVel = -8.0f;
            }
        }
        else if (c=="rps") {
            const char* choices[]={"ROCK","PAPER","SCISSORS"};
            rpsChoice=choices[random(0,3)]; soul.setState(STATE_GAME_RPS);
            rpsActive=true; rpsEnd=millis()+4000;
            audio.playHD(voice_rps_data,sizeof(voice_rps_data),3,"1,2,3 SHOOT!");
        }
        else if (c=="8ball") {
            const char* answers[]={"YES!","NO WAY!","MAYBE!","TRY AGAIN","ABSOLUTELY"};
            magic8Ans=answers[random(0,5)]; magic8Active=true; magic8End=millis()+4500;
            audio.playChirp(500,1500,200);
        }
        else if (c=="snack") {
            soul.feedSnack(); soul.setState(STATE_SNACK_FEEDING);
            snackActive=true; snackEnd=millis()+3000;
            audio.playHD(voice_snack_data,sizeof(voice_snack_data),3,"YUM!");
        }
    }
    server.send(200,"text/plain","OK");
}

void handleStatus() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String j = "{";
    j += "\"sta_connected\":"   + String(net.isStaConnected()?"true":"false") + ",";
    j += "\"sta_ip\":\""        + net.getStaIP() + "\",";
    j += "\"ap_ip\":\""         + net.getApIP() + "\",";
    j += "\"time_str\":\""      + brain.currentTime + "\",";
    j += "\"date_str\":\""      + brain.currentDate + "\",";
    j += "\"affection\":"        + String(soul.getAffection()) + ",";
    j += "\"energy\":"           + String(soul.getEnergy()) + ",";
    j += "\"hunger\":"           + String(soul.getHunger()) + ",";
    j += "\"servo_angle\":"      + String((int)servo.getCurrent()) + ",";
    j += "\"temp_c\":"           + String(brain.currentTempC) + ",";
    j += "\"humidity\":"         + String(brain.currentHumidity) + ",";
    j += "\"weather\":\""        + brain.currentWeather + "\",";
    j += "\"volume\":"           + String(audio.getVolume()) + ",";
    j += "\"is_muted\":"         + String(audio.isMuted()?"true":"false") + ",";
    j += "\"mic_enabled\":"      + String(sensors.getSoundEnabled()?"true":"false") + ",";
    j += "\"flappy_hi\":"        + String(flappyHiScore) + ",";
    j += "\"key_count\":"        + String(brain.getKeyCount()) + ",";
    j += "\"active_key\":"       + String(brain.getActiveKeyIndex()+1) + ",";
    j += "\"owner_name\":\""     + brain.getOwnerName() + "\",";
    j += "\"auto_talk_min\":"    + String(soul.getAutoTalkIntervalMinutes()) + ",";
    // Clean last reply of emotion tags for display
    String lr = brain.getLastReply();
    int cb = lr.indexOf(']');
    if (cb != -1) { lr = lr.substring(cb+1); lr.trim(); }
    int rm = lr.indexOf("[REMEMBER");
    if (rm != -1) lr = lr.substring(0, rm);
    lr.trim();
    j += "\"last_reply\":\""     + lr + "\"";
    j += "}";
    server.send(200,"application/json",j);
}

void handleWifiScan() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.send(200,"application/json",net.scanNetworks());
}

void handleWifiSave() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String body = server.arg("plain");
    int si = body.indexOf("\"ssid\":\"");
    int pi = body.indexOf("\"pass\":\"");
    if (si == -1) { server.send(400,"text/plain","Bad request"); return; }
    String ssid = body.substring(si+8, body.indexOf('"', si+8));
    String pass = (pi!=-1) ? body.substring(pi+8, body.indexOf('"', pi+8)) : "";
    net.saveStaCreds(ssid, pass);
    net.connectStation(ssid, pass, gmtOffsetHours);
    server.send(200,"text/plain","OK");
}

void handleGeminiKey() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String raw = server.arg("plain");
    raw.replace("\n",","); raw.replace("\r","");
    brain.saveKeyPool(raw);
    server.send(200,"text/plain","OK");
}

void handleGeminiAsk() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String prompt = server.arg("plain");
    if (prompt.length() == 0) prompt = server.arg("q");
    if (prompt.length() == 0) { server.send(400,"text/plain","Empty prompt"); return; }
    String reply = brain.askGemini(prompt);
    // Clean for HTTP response
    int cb = reply.indexOf(']');
    if (cb != -1) { reply = reply.substring(cb+1); reply.trim(); }
    int rm = reply.indexOf("[REMEMBER");
    if (rm != -1) reply = reply.substring(0, rm);
    reply.trim();
    server.send(200,"text/plain",reply);
}

void handleSettingsSave() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String body = server.arg("plain");
    // Parse tz, lat, lon from JSON
    int ti = body.indexOf("\"tz\":");
    int lai = body.indexOf("\"lat\":");
    int loi = body.indexOf("\"lon\":");
    if (ti != -1)  gmtOffsetHours = (int)body.substring(ti+5, body.indexOf(',', ti)).toFloat();
    if (lai != -1) userLat = body.substring(lai+6, body.indexOf(',', lai)).toFloat();
    if (loi != -1) userLon = body.substring(loi+6, body.indexOf(',', loi)).toFloat();
    Preferences p; p.begin("piku",false);
    p.putInt("gmt_offset",(int)gmtOffsetHours);
    p.putFloat("user_lat",userLat);
    p.putFloat("user_lon",userLon);
    p.end();
    server.send(200,"text/plain","OK");
}

void handleSync() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    net.syncNTP(gmtOffsetHours);
    int t=brain.currentTempC, h=brain.currentHumidity;
    String cond=brain.currentWeather;
    net.fetchWeather(userLat,userLon,&t,&h,&cond);
    brain.currentTempC=t; brain.currentHumidity=h; brain.currentWeather=cond;
    server.send(200,"text/plain","OK");
}

void handleBillboard() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String msg = server.arg("plain");
    if (msg.length() > 0) disp.startScrollMessage(msg, "MSG");
    server.send(200,"text/plain","OK");
}

void handleOwnerName() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String name = server.arg("plain");
    name.trim();
    if (name.length() > 0 && name.length() < 32) {
        brain.setOwnerName(name);
        brain.completeOnboarding();
        audio.playHD(voice_hello_data, sizeof(voice_hello_data), 2, ("Hi, "+name+"!").c_str());
        soul.triggerEmotion(EMOTION_HELLO, 80, 3000);
        server.send(200,"text/plain","OK");
    } else {
        server.send(400,"text/plain","Invalid name");
    }
}

void handleOwnerReset() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    Preferences p; p.begin("piku",false);
    p.remove("owner_name"); p.remove("owner_facts"); p.remove("onboarding");
    p.end();
    server.send(200,"text/plain","OK");
}

// ─── setup() — Hardware init + FreeRTOS launch ───────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(400);
    Serial.println(F("\n=== PIKU 2.0 BOOTING ==="));

    // 1. Load NVS settings
    Preferences prefs; prefs.begin("piku", true);
    gmtOffsetHours  = prefs.getInt("gmt_offset", DEFAULT_GMT_OFFSET);
    userLat         = prefs.getFloat("user_lat", 23.8103f);
    userLon         = prefs.getFloat("user_lon", 90.4125f);
    masterVolume    = prefs.getInt("volume", 80);
    soundEnabled    = prefs.getBool("mic_en", false);
    flappyHiScore   = prefs.getInt("flappy_hi", 0);
    String storedSSID = prefs.getString("sta_ssid", "");
    String storedPass = prefs.getString("sta_pass", "");
    int    autoTalkMin = prefs.getInt("auto_talk", 10);
    prefs.end();

    // 2. Init modules
    servo.init();
    audio.init(masterVolume);
    disp.init();
    sensors.init(&audio);
    soul.init(&disp, &audio, &servo);
    brain.init(&soul, &audio);
    soul.setAutoTalkInterval(autoTalkMin);
    sensors.setSoundEnabled(soundEnabled);

    // 3. Wire sensor callbacks to SoulEngine
    sensors.onTouchShort([]()     { soul.onTouchShort(); });
    sensors.onTouchSustained([]() { soul.onTouchSustained(); });
    sensors.onTouchOverpet([]()   { soul.onTouchOverpet(); });
    sensors.onDoubleClap([]()     { soul.onDoubleClap(); });

    // 4. Start WiFi
    net.init(AP_DEFAULT_SSID, AP_DEFAULT_PASS, MDNS_HOSTNAME);
    if (storedSSID.length() > 0) {
        net.connectStation(storedSSID, storedPass, gmtOffsetHours);
    }

    // 5. Web server routes
    server.on("/",                  HTTP_GET,  handleRoot);
    server.on("/api",               HTTP_GET,  handleCommand);
    server.on("/api/status",        HTTP_GET,  handleStatus);
    server.on("/api/gemini/ask",    HTTP_POST, handleGeminiAsk);
    server.on("/api/gemini/ask",    HTTP_GET,  handleGeminiAsk);
    server.on("/api/gemini/key",    HTTP_POST, handleGeminiKey);
    server.on("/api/wifi/scan",     HTTP_POST, handleWifiScan);
    server.on("/api/wifi/save",     HTTP_POST, handleWifiSave);
    server.on("/api/settings/save", HTTP_POST, handleSettingsSave);
    server.on("/api/billboard",     HTTP_POST, handleBillboard);
    server.on("/api/sync",          HTTP_POST, handleSync);
    server.on("/api/owner/name",    HTTP_POST, handleOwnerName);
    server.on("/api/owner/reset",   HTTP_POST, handleOwnerReset);
    server.begin();
    Serial.println(F("[HTTP] Server started on port 80"));

    // 6. Audio task on Core 1 (low priority), then start other tasks
    audio.startTask();

    // 7. Launch Core 0 and Core 1 FreeRTOS tasks
    xTaskCreatePinnedToCore(networkTask, "NetTask",  8192, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(soulTask,    "SoulTask", 6144, nullptr, 2, nullptr, 1);

    // 8. Startup greeting
    delay(500);
    audio.playHD(voice_hello_data, sizeof(voice_hello_data), 2, "Hi! I am Piku!");
    soul.triggerEmotion(EMOTION_HELLO, 80, 3000);

    Serial.println(F("=== PIKU 2.0 READY ==="));
}

void loop() {
    // Empty — all work done in FreeRTOS tasks
    vTaskDelay(portMAX_DELAY);
}
```

- [ ] **Step 2: Full build verification**

```bash
pio run -e esp32dev 2>&1 | tail -20
```
Expected: build succeeds with "Huge APP" partition. RAM usage < 70%. Binary should be 1.5-2.5 MB.

- [ ] **Step 3: Flash to ESP32**

```bash
pio run -e esp32dev --target upload
pio device monitor --baud 115200
```

Expected serial output:
```
=== PIKU 2.0 BOOTING ===
[WiFi] AP: Piku-WiFi  IP: 192.168.4.1
[mDNS] http://piku.local
[HTTP] Server started on port 80
[Brain] Loaded N Gemini keys
=== PIKU 2.0 READY ===
```
Then OLED shows "PIKU AI / WAKING UP..." and plays hello audio.

- [ ] **Step 4: Functional smoke test**

Connect phone to "Piku-WiFi" and open `http://192.168.4.1`:
- [ ] Status tab shows vitals bars
- [ ] Controls tab: tap "Hello" — OLED face animates, servo moves, audio plays
- [ ] Controls tab: tap "Party" — DJ equalizer eye appears with wiggle
- [ ] Chat tab: type "How are you?" and tap Ask — Gemini responds (requires WiFi to router)
- [ ] Settings tab: enter Gemini key and save
- [ ] Spontaneous talk fires after configured interval (test with 5 min setting)
- [ ] Double-clap triggers party mode (if mic enabled)
- [ ] Petting touch sensor triggers cat/love expressions
- [ ] Flappy Bird game renders on OLED and servo + web server remain responsive during play

- [ ] **Step 5: Save flappy high score to NVS**

In `handleCommand()` after `flap_start` / `flap_jump` logic, after flappyHiScore updates:
```cpp
Preferences p; p.begin("piku",false); p.putInt("flappy_hi", flappyHiScore); p.end();
```
Add this inside the flap_jump handler when `flappyHiScore` changes.

- [ ] **Step 6: Commit final integration**

```bash
git add Piku/Piku.ino
git commit -m "feat(piku2): main FreeRTOS dual-core integration — all modules wired, web server, game logic"
```

---

## Post-Integration Checklist

- [ ] Verify OLED face morphs smoothly (not jumps) between IDLE → LOVE → CAT
- [ ] Verify audio plays while web server responds (no freeze during voice clips)
- [ ] Verify spontaneous talk fires autonomously at configured interval
- [ ] Verify first-boot onboarding: if `onboardingDone=false`, Piku greets with "Hi! What's your name?"
- [ ] Verify `[REMEMBER:]` fact extraction persists across reboot (check NVS)
- [ ] Verify Flappy Bird high score saves to NVS across reboot
- [ ] Verify double-clap party mode works with mic enabled
- [ ] Verify page loads in < 2 seconds on mobile over AP hotspot
