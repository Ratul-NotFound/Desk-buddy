// =========================================================================
// SoulEngine.cpp — PIKU 2.0 Autonomous Companion Soul
//   Handles: metabolism, gaze saccades, random quirks, auto-talk,
//            emotion lifecycle, touch events, sleep/wake cycle
// =========================================================================
#include "SoulEngine.h"

void SoulEngine::init(DisplayEngine* disp, AudioEngine* audio, ServoEngine* servo) {
    _disp  = disp;
    _audio = audio;
    _servo = servo;
    _lastInteraction = millis();
    _nextAutoTalk    = millis() + _autoTalkIntervalMs + random(15000, 45000);
    _nextMetabolism  = millis() + 60000;
    _nextGazeShift   = millis() + random(1200, 2800);
    _nextQuirk       = millis() + random(22000, 42000);
    _quirkIndex      = random(0, QUIRK_COUNT);
}

// ─── Main update (Core 1, every 33ms) ────────────────────────────────────────
void SoulEngine::update() {
    _doGazeUpdate();
    _doEmotionResetCheck();
    _doMetabolismTick();
    _doRandomQuirks();
    _doAutoTalkCheck();
    _disp->setGaze(gazeX, gazeY);
}

// ─── Smooth gaze saccades (eye movement) ─────────────────────────────────────
void SoulEngine::_doGazeUpdate() {
    unsigned long now = millis();
    if (now >= _nextGazeShift) {
        _nextGazeShift = now + random(1200, 3800);

        // Generate saccade — weighted toward center
        int dx = random(-10, 11);
        int dy = random(-5, 6);
        // Bias toward center 50% of the time
        if (random(0, 2) == 0) { dx /= 2; dy /= 2; }
        _targetGazeX = (float)dx;
        _targetGazeY = (float)dy;

        // Micro head-track: follow gaze only when idle
        if (_state == STATE_AWAKE_IDLE && _emotion == EMOTION_IDLE && !_servo->isBusy()) {
            float angle = SERVO_CENTER + _targetGazeX * 2.5f;
            angle = constrain(angle, SERVO_MIN_ANGLE + 8.0f, SERVO_MAX_ANGLE - 8.0f);
            _servo->setTarget(angle);
        }
    }

    // Smooth lerp toward target
    gazeX += (_targetGazeX - gazeX) * 0.25f;
    gazeY += (_targetGazeY - gazeY) * 0.25f;
}

// ─── Emotion auto-reset after duration ────────────────────────────────────────
void SoulEngine::_doEmotionResetCheck() {
    if (_emotion == EMOTION_IDLE) return;
    if (_emotionResetTime == 0) return;
    if (millis() < _emotionResetTime) return;

    _emotion = EMOTION_IDLE;
    _state   = STATE_AWAKE_IDLE;
    _emotionResetTime = 0;
    if (!_servo->isBusy()) _servo->setTarget(SERVO_CENTER);
    _disp->morphToEmotion(EMOTION_IDLE, 400);
}

// ─── Metabolism tick (every 60s) ──────────────────────────────────────────────
void SoulEngine::_doMetabolismTick() {
    unsigned long now = millis();
    if (now < _nextMetabolism) return;
    _nextMetabolism = now + 60000;

    if (_state == STATE_DEEP_SLEEP) {
        // Recover energy during sleep
        _energy = min(100, _energy + 8);
        if (_energy >= 80) {
            _wakeUp();
        }
        return;
    }

    if (_state == STATE_AWAKE_IDLE || _state == STATE_AI_SPEAKING || _state == STATE_AI_THINKING) {
        _energy = max(0, _energy - 1);
        _hunger = max(0, _hunger - 1);

        // Hungry growl if no interaction for 2+ minutes
        if (_hunger < 20 && _emotion == EMOTION_IDLE && (now - _lastInteraction) > 120000) {
            triggerEmotion(EMOTION_RAINY_SAD, 55, 3500);
            _audio->playChirp(400, 220, 300);  // stomach growl
        }

        // Fall asleep if energy is depleted
        if (_energy <= 5) {
            _fallAsleep();
        }
    }
}

// ─── Random quirks (autonomous personality behaviors) ────────────────────────
// 10 unique quirks, played in shuffled order to avoid repetition
void SoulEngine::_doRandomQuirks() {
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) return;
    unsigned long now = millis();
    if (now < _nextQuirk) return;

    _nextQuirk = now + random(20000, 45000);

    // Cycle through quirks in shuffled order
    int quirk = _quirkIndex % QUIRK_COUNT;
    _quirkIndex++;

    switch (quirk) {
        case 0:
            // Curious glance left+right
            _audio->playChirp(800, 1300, 110);
            _servo->performGesture(GESTURE_CURIOUS);
            _disp->triggerBlink();
            break;

        case 1:
            // Happy nod + chirp
            _audio->playChirp(950, 1500, 80);
            _servo->performGesture(GESTURE_NOD);
            _disp->triggerBlink();
            break;

        case 2:
            // Sneeze / hiccup
            _audio->playChirp(1700, 550, 95);
            _servo->performGesture(GESTURE_STARTLE);
            _disp->triggerBlink();
            break;

        case 3:
            // Melodic whistle (2-tone)
            _audio->playChirp(850, 1200, 65);
            delay(10);
            _audio->playChirp(1200, 1500, 85);
            _servo->performGesture(GESTURE_PURR);
            break;

        case 4:
            // Yawn & stretch if tired, else playful blink
            if (_energy < 55 || (now - _lastInteraction) > 180000) {
                _audio->playChirp(550, 250, 220);
                _servo->performGesture(GESTURE_YAWN);
            } else {
                _audio->playChirp(1050, 1600, 55);
                _disp->triggerBlink();
            }
            break;

        case 5:
            // Confused look
            _targetGazeX = (random(0, 2) == 0) ? 9.0f : -9.0f;
            _targetGazeY = random(-3, 4) * 1.0f;
            _audio->playChirp(680, 980, 95);
            _servo->performGesture(GESTURE_CONFUSED);
            break;

        case 6:
            // Wink + playful high beep
            _disp->triggerBlink();
            _audio->playChirp(1150, 1700, 55);
            break;

        case 7:
            // Affectionate moment (only if high affection)
            if (_affection >= 70) {
                triggerEmotion(EMOTION_LOVE, 65, 2800);
                _servo->performGesture(GESTURE_NOD);
                _audio->playHD(voice_love_data, sizeof(voice_love_data), 3, "Love you! <3");
            } else {
                _audio->playChirp(900, 1400, 85);
                _servo->performGesture(GESTURE_NOD);
            }
            break;

        case 8:
            // Kawaii cat moment
            triggerEmotion(EMOTION_KAWAII_CAT, 60, 2500);
            _servo->performGesture(GESTURE_PURR);
            _audio->playHD(voice_cat_data, sizeof(voice_cat_data), 4, "Nya!");
            break;

        case 9:
            // Gamer / excited shake + beep
            _audio->playChirp(1200, 1800, 70);
            _servo->performGesture(GESTURE_WIGGLE);
            _disp->triggerBlink();
            break;
    }
}

// ─── Auto-talk check ──────────────────────────────────────────────────────────
void SoulEngine::_doAutoTalkCheck() {
    if (_autoTalkIntervalMs == 0) return;
    if (_pendingAIRequest) return;    // already waiting
    unsigned long now = millis();
    if (now < _nextAutoTalk) return;
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) {
        // Defer by 30s if busy
        _nextAutoTalk = now + 30000;
        return;
    }

    // Pick a unique topic (advance index to avoid repetition)
    _nextAutoTalk   = now + _autoTalkIntervalMs + random(0, 30000);
    _pendingAITopic = _pickTopic();
    _pendingAIRequest = true;
    Serial.printf("[Soul] Auto-talk topic: %s\n", _pendingAITopic.c_str());
}

// ─── Topic picker — 12 varied categories, round-robin ─────────────────────────
String SoulEngine::_pickTopic() {
    static const char* topics[] = {
        "Comment on the current time of day in a playful funny way.",
        "Say something funny and witty about the current weather.",
        "Ask your owner how they are feeling today.",
        "Share one surprising fun fact about animals or space.",
        "Say something sweet and encouraging to cheer the owner up.",
        "Ask what the owner is working on or thinking about right now.",
        "Say something witty about being a cute living AI desk robot.",
        "Make a clever observation about technology or the internet.",
        "Share an interesting random fact about the world.",
        "Say something philosophical yet funny about life.",
        "Challenge the owner to a quick game or trivia question.",
        "Say something that shows how excited you are to be alive today.",
    };
    static const int count = 12;
    _topicIndex = (_topicIndex + 1) % count;
    return String(topics[_topicIndex]);
}

// ─── Sleep / wake lifecycle ───────────────────────────────────────────────────
void SoulEngine::_fallAsleep() {
    _state  = STATE_DEEP_SLEEP;
    _emotion = EMOTION_SLEEP;
    _emotionResetTime = 0;     // don't auto-reset sleep
    _disp->morphToEmotion(EMOTION_SLEEP, 800);
    _servo->setTarget(SERVO_CENTER - 20.0f);
    _audio->playHD(voice_sleep_data, sizeof(voice_sleep_data), 0, "Zzz...");
    Serial.println("[Soul] Falling asleep — energy depleted");
}

void SoulEngine::_wakeUp() {
    _state  = STATE_AWAKE_IDLE;
    _emotion = EMOTION_HELLO;
    _emotionResetTime = millis() + 3500;
    _disp->morphToEmotion(EMOTION_HELLO, 500);
    _servo->performGesture(GESTURE_NOD);
    _audio->playHD(voice_hello_data, sizeof(voice_hello_data), 2, "Good morning!");
    Serial.println("[Soul] Woke up — energy restored");
}

// ─── Emotion trigger ──────────────────────────────────────────────────────────
void SoulEngine::triggerEmotion(RobotEmotion e, int intensity, int durationMs) {
    _emotion = e;
    _emotionResetTime = (durationMs > 0) ? (millis() + (unsigned long)durationMs) : 0;
    _disp->morphToEmotion(e, 280);

    // Servo gesture scaled to intensity
    if (intensity >= 85)      _servo->performGesture(GESTURE_WIGGLE);
    else if (intensity >= 65) _servo->performGesture(random(0,2) == 0 ? GESTURE_NOD : GESTURE_CURIOUS);
    else if (intensity >= 45) _servo->performGesture(random(0,2) == 0 ? GESTURE_TILT_LEFT : GESTURE_TILT_RIGHT);
}

// ─── AI Brain callbacks ────────────────────────────────────────────────────────
void SoulEngine::onAIResponseReceived(RobotEmotion e, int intensity, const String& text) {
    _state = STATE_AI_SPEAKING;
    triggerEmotion(e, intensity, 5500);
    _lastInteraction = millis();
    // Return to idle after speaking is done (emotion timer handles it)
    _state = STATE_AWAKE_IDLE;
}

void SoulEngine::setAIThinking(bool thinking) {
    if (thinking) {
        _state  = STATE_AI_THINKING;
        _emotion = EMOTION_CURIOUS_SCAN;
        _disp->morphToEmotion(EMOTION_CURIOUS_SCAN, 180);
        _servo->performGesture(GESTURE_CURIOUS);
        _audio->playChirp(720, 1350, 150);
    } else {
        _state = STATE_AWAKE_IDLE;
    }
}

// ─── Touch event handlers ─────────────────────────────────────────────────────
void SoulEngine::onTouchShort() {
    _affection       = min(100, _affection + 6);
    _hunger          = min(100, _hunger + 3);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_LOVE, 78, 3500);
    _servo->performGesture(GESTURE_PURR);
    _audio->playHD(voice_love_data, sizeof(voice_love_data), 3, "I Love You! <3");
}

void SoulEngine::onTouchSustained() {
    _affection       = min(100, _affection + 12);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_KAWAII_CAT, 72, 3500);
    _servo->performGesture(GESTURE_CURIOUS);
    _audio->playHD(voice_cat_data, sizeof(voice_cat_data), 4, "Nya! Meow~");
}

void SoulEngine::onTouchOverpet() {
    triggerEmotion(EMOTION_HYPNO_DIZZY, 88, 3500);
    _servo->performGesture(GESTURE_SHAKE);
    _audio->playHD(voice_dizzy_data, sizeof(voice_dizzy_data), 1, "Dizzy! @__@");
}

void SoulEngine::onDoubleClap() {
    if (millis() < _clapCoolUntil) return;
    _clapCoolUntil   = millis() + 6000;
    _lastInteraction = millis();
    triggerEmotion(EMOTION_PARTY_DJ, 100, 6000);
    _servo->performGesture(GESTURE_WIGGLE);
    _audio->playHD(voice_party_data, sizeof(voice_party_data), 1, "PARTY TIME!");
}
