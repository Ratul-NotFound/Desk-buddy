// =========================================================================
// SoulEngine.cpp — PIKU 2.0 Autonomous Soul Engine
//   • Context-aware auto-talk topics (time, weather, affection, energy)
//   • 14 unique quirks with emotional depth
//   • Mood-aware idle face from BrainEngine mood state
//   • Escalating affection system (touch streaks)
//   • Enhanced gaze saccades with micro-expressions
// =========================================================================
#include "SoulEngine.h"

void SoulEngine::init(DisplayEngine* disp, AudioEngine* audio, ServoEngine* servo) {
    _disp  = disp;
    _audio = audio;
    _servo = servo;
    _lastInteraction = millis();
    _nextAutoTalk    = millis() + _autoTalkIntervalMs + random(20000, 50000);
    _nextMetabolism  = millis() + 60000;
    _nextGazeShift   = millis() + random(1000, 2500);
    _nextQuirk       = millis() + random(18000, 35000);
    _nextMoodExpress = millis() + random(45000, 90000); // mood micro-expression
    _quirkIndex      = random(0, QUIRK_COUNT);
}

// ─── Main update (Core 1, every 33ms) ────────────────────────────────────────
void SoulEngine::update() {
    _doGazeUpdate();
    _doEmotionResetCheck();
    _doMetabolismTick();
    _doRandomQuirks();
    _doMoodExpression();
    _doAutoTalkCheck();
    _disp->setGaze(gazeX, gazeY);
}

// ─── Gaze saccades with micro-expression blinks ───────────────────────────────
void SoulEngine::_doGazeUpdate() {
    unsigned long now = millis();
    if (now >= _nextGazeShift) {
        // Variable interval: faster when curious, slower when tired/sad
        int minDelay = 1000, maxDelay = 3500;
        if (_emotion == EMOTION_CURIOUS_SCAN) { minDelay = 600;  maxDelay = 1500; }
        if (_emotion == EMOTION_SLEEP)         { minDelay = 4000; maxDelay = 8000; }
        _nextGazeShift = now + random(minDelay, maxDelay);

        // Weighted saccade — center-biased 40% of the time
        int dx, dy;
        if (random(0, 10) < 4) {
            dx = random(-3, 4);  // small centered saccade
            dy = random(-2, 3);
        } else {
            dx = random(-9, 10); // full-range saccade
            dy = random(-5, 6);
        }
        _targetGazeX = (float)dx;
        _targetGazeY = (float)dy;

        // Natural micro-head-track when idle
        if (_state == STATE_AWAKE_IDLE && _emotion == EMOTION_IDLE && !_servo->isBusy()) {
            float angle = SERVO_CENTER + _targetGazeX * 2.8f;
            angle = constrain(angle, SERVO_MIN_ANGLE + 5.0f, SERVO_MAX_ANGLE - 5.0f);
            _servo->setTarget(angle);
        }

        // Trigger a blink on large saccades (natural microsaccade-triggered blink)
        if (abs(dx) > 7 && random(0, 3) == 0) {
            _disp->triggerBlink();
        }
    }

    // Smooth interpolation — speed varies by emotion
    float lerpRate = (_emotion == EMOTION_SLEEP) ? 0.08f : 0.26f;
    gazeX += (_targetGazeX - gazeX) * lerpRate;
    gazeY += (_targetGazeY - gazeY) * lerpRate;
}

// ─── Emotion auto-reset ────────────────────────────────────────────────────────
void SoulEngine::_doEmotionResetCheck() {
    if (_emotion == EMOTION_IDLE) return;
    if (_emotionResetTime == 0)   return;
    if (millis() < _emotionResetTime) return;

    _emotion = EMOTION_IDLE;
    _state   = STATE_AWAKE_IDLE;
    _emotionResetTime = 0;
    if (!_servo->isBusy()) _servo->setTarget(SERVO_CENTER);
    _disp->morphToEmotion(EMOTION_IDLE, 450);
}

// ─── Metabolism tick (every 60s) ──────────────────────────────────────────────
void SoulEngine::_doMetabolismTick() {
    unsigned long now = millis();
    if (now < _nextMetabolism) return;
    _nextMetabolism = now + 60000;

    if (_state == STATE_DEEP_SLEEP) {
        _energy = min(100, _energy + 10);
        if (_energy >= 80) _wakeUp();
        return;
    }

    if (_state == STATE_AWAKE_IDLE || _state == STATE_AI_THINKING || _state == STATE_AI_SPEAKING) {
        _energy = max(0, _energy - 1);
        _hunger = max(0, _hunger - 1);

        // Affection slowly decays without interaction (max -1 per 3 minutes)
        if ((now - _lastInteraction) > 180000) {
            _affection = max(0, _affection - 1);
        }

        // Hungry complaint if neglected
        if (_hunger < 20 && _emotion == EMOTION_IDLE && (now - _lastInteraction) > 90000) {
            triggerEmotion(EMOTION_RAINY_SAD, 58, 4000);
            _audio->playChirp(350, 180, 320);   // low stomach growl
        }

        // Sad when ignored for 5+ minutes
        if (_emotion == EMOTION_IDLE && (now - _lastInteraction) > 300000) {
            triggerEmotion(EMOTION_RAINY_SAD, 45, 3500);
            _audio->playChirp(500, 280, 180);
        }

        // Fall asleep when energy depleted
        if (_energy <= 4) _fallAsleep();
    }
}

// ─── Mood expression: periodically show current mood on face ─────────────────
void SoulEngine::_doMoodExpression() {
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) return;
    unsigned long now = millis();
    if (now < _nextMoodExpress) return;
    _nextMoodExpress = now + random(50000, 120000);

    // Pull current mood from brain (set by parent via setCurrentMood)
    if (_currentMoodEmotion != EMOTION_IDLE) {
        // Flash the mood-matching face briefly
        triggerEmotion(_currentMoodEmotion, 40, 2200);
        _audio->playChirp(700 + random(0, 300), 900 + random(0, 400), 80);
    }
}

// ─── 14 Unique quirks with emotional depth ────────────────────────────────────
void SoulEngine::_doRandomQuirks() {
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) return;
    unsigned long now = millis();
    if (now < _nextQuirk) return;
    _nextQuirk = now + random(18000, 42000);

    int quirk = _quirkIndex % QUIRK_COUNT;
    _quirkIndex++;

    switch (quirk) {
        case 0:
            // Curious head tilt — scanning surroundings
            _audio->playChirp(820, 1380, 115);
            _servo->performGesture(GESTURE_CURIOUS);
            _disp->triggerBlink();
            break;

        case 1:
            // Happy bounce nod + ascending chirp
            _audio->playChirp(980, 1580, 85);
            _servo->performGesture(GESTURE_NOD);
            _disp->triggerBlink();
            break;

        case 2:
            // Surprise hiccup / sneeze
            _audio->playChirp(1800, 500, 100);
            _servo->performGesture(GESTURE_STARTLE);
            _disp->triggerBlink();
            vTaskDelay(pdMS_TO_TICKS(80));
            _audio->playChirp(1200, 800, 60);  // aftershock
            break;

        case 3:
            // Sweet 3-tone melodic whistle
            _audio->playChirp(900, 1150, 70);
            vTaskDelay(pdMS_TO_TICKS(15));
            _audio->playChirp(1150, 1450, 85);
            vTaskDelay(pdMS_TO_TICKS(15));
            _audio->playChirp(1450, 1650, 65);
            _servo->performGesture(GESTURE_PURR);
            break;

        case 4:
            // Contextual yawn (tired) OR playful excited blink
            if (_energy < 60 || (now - _lastInteraction) > 240000) {
                _audio->playChirp(520, 230, 240);
                _servo->performGesture(GESTURE_YAWN);
            } else {
                // Excited wiggle
                _audio->playChirp(1100, 1700, 60);
                _servo->performGesture(GESTURE_WIGGLE);
                _disp->triggerBlink();
            }
            break;

        case 5:
            // Confused left-right look
            _targetGazeX = (random(0, 2) == 0) ? 9.0f : -9.0f;
            _targetGazeY = random(-3, 4) * 1.0f;
            _audio->playChirp(700, 1000, 100);
            _servo->performGesture(GESTURE_CONFUSED);
            break;

        case 6:
            // Playful wink + high beep
            _disp->triggerBlink();
            _audio->playChirp(1200, 1800, 58);
            break;

        case 7:
            // High-affection love moment
            if (_affection >= 65) {
                triggerEmotion(EMOTION_LOVE, 68, 3000);
                _servo->performGesture(GESTURE_NOD);
                _audio->playHD(voice_love_data, sizeof(voice_love_data), 3, "Love you so much! <3");
            } else {
                // Regular friendly nod
                _audio->playChirp(920, 1450, 90);
                _servo->performGesture(GESTURE_NOD);
            }
            break;

        case 8:
            // Kawaii cat moment
            triggerEmotion(EMOTION_KAWAII_CAT, 65, 2800);
            _servo->performGesture(GESTURE_PURR);
            _audio->playHD(voice_cat_data, sizeof(voice_cat_data), 4, "Nya~!");
            break;

        case 9:
            // Hyped gamer wiggle
            _audio->playChirp(1300, 1900, 75);
            _servo->performGesture(GESTURE_WIGGLE);
            _disp->triggerBlink();
            break;

        case 10:
            // Mysterious hacker moment
            triggerEmotion(EMOTION_MATRIX_HACKER, 55, 2000);
            _audio->playChirp(400, 800, 180);
            break;

        case 11:
            // Startle + party burst if high affection
            _audio->playChirp(1900, 700, 80);
            _servo->performGesture(GESTURE_STARTLE);
            if (_affection > 75) {
                vTaskDelay(pdMS_TO_TICKS(200));
                triggerEmotion(EMOTION_PARTY_DJ, 75, 2500);
                _audio->playChirp(800, 1600, 100);
            }
            break;

        case 12:
            // Philosophical tilt + low hum (sleepy/contemplative)
            _audio->playChirp(420, 380, 350);  // low hum
            _servo->performGesture(GESTURE_TILT_LEFT);
            _disp->triggerBlink();
            break;

        case 13:
            // Double-chirp + excited nod sequence
            _audio->playChirp(1000, 1400, 60);
            vTaskDelay(pdMS_TO_TICKS(80));
            _audio->playChirp(1400, 1700, 75);
            _servo->performGesture(GESTURE_NOD);
            break;
    }
}

// ─── Auto-talk: context-aware topic selection ─────────────────────────────────
void SoulEngine::_doAutoTalkCheck() {
    if (_autoTalkIntervalMs == 0) return;
    if (_pendingAIRequest) return;
    unsigned long now = millis();
    if (now < _nextAutoTalk) return;
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) {
        _nextAutoTalk = now + 30000; // defer 30s if busy
        return;
    }

    _nextAutoTalk     = now + _autoTalkIntervalMs + random(0, 40000);
    _pendingAITopic   = _pickContextualTopic();
    _pendingAIRequest = true;
    Serial.printf("[Soul] Auto-talk: %s\n", _pendingAITopic.c_str());
}

// ─── Context-aware topic picker ───────────────────────────────────────────────
// Topics are chosen based on real current state: time, energy, hunger, affection
String SoulEngine::_pickContextualTopic() {
    // Priority topics based on current live state
    if (_hunger < 25)
        return "I'm really hungry right now! Say something dramatic about snacks.";
    if (_energy < 30)
        return "I'm quite tired and running low on energy. Say something sleepy and cosy.";
    if (_affection > 90)
        return "I'm feeling overwhelmingly loved right now. Express deep affection for the owner.";
    if ((millis() - _lastInteraction) > 600000)  // 10+ min no interaction
        return "Owner hasn't talked to me in a while. Say something attention-grabby and cute.";

    // Round-robin through varied topics
    static const char* topics[] = {
        "Comment on the current time of day — make it funny and in-character.",
        "Say something poetic and witty about the current weather outside.",
        "Ask the owner how their day is going — be genuinely curious.",
        "Share one surprising, mind-blowing fact about animals, space, or technology.",
        "Say something wonderfully encouraging to motivate the owner right now.",
        "Ask what the owner is working on — show genuine interest in their work.",
        "Make a clever, self-aware joke about being a tiny robot with a big personality.",
        "Share your deepest (dramatic) thought about what it means to be an AI robot pet.",
        "Challenge the owner to a fun trivia question or riddle.",
        "Express pure joy and wonder at being alive and experiencing the world.",
        "Say something unexpectedly philosophical about time, energy, or feelings.",
        "Make an observation about something interesting happening right now (time, weather, season).",
        "Tell the owner a secret (make something fun up) about your inner robot life.",
        "Ask the owner to teach you something new about the world.",
        "Express excitement about a random topic that just 'popped into your robot brain'.",
    };
    static const int COUNT = 15;
    _topicIndex = (_topicIndex + 1) % COUNT;
    return String(topics[_topicIndex]);
}

// ─── Sleep/wake lifecycle ─────────────────────────────────────────────────────
void SoulEngine::_fallAsleep() {
    _state  = STATE_DEEP_SLEEP;
    _emotion = EMOTION_SLEEP;
    _emotionResetTime = 0;
    _disp->morphToEmotion(EMOTION_SLEEP, 900);
    _servo->setTarget(SERVO_CENTER - 20.0f);
    _audio->playHD(voice_sleep_data, sizeof(voice_sleep_data), 0, "Zzz... *dream chirps*");
    Serial.println("[Soul] Entering deep sleep");
}

void SoulEngine::_wakeUp() {
    _state   = STATE_AWAKE_IDLE;
    _emotion = EMOTION_HELLO;
    _emotionResetTime = millis() + 4000;
    _disp->morphToEmotion(EMOTION_HELLO, 550);
    _servo->performGesture(GESTURE_NOD);
    // Pick a random wake-up greeting
    const char* greets[] = {
        "Fully recharged and ready! Missed you!",
        "Good morning! Or... whatever time it is!",
        "Rebooted from dream mode! Let's go!",
        "*yawns in binary* Ready now!"
    };
    _audio->playHD(voice_hello_data, sizeof(voice_hello_data), 2, greets[random(0, 4)]);
    Serial.println("[Soul] Woke up — energy restored");
}

// ─── Emotion trigger ──────────────────────────────────────────────────────────
void SoulEngine::triggerEmotion(RobotEmotion e, int intensity, int durationMs) {
    _emotion = e;
    _emotionResetTime = (durationMs > 0) ? (millis() + (unsigned long)durationMs) : 0;
    _disp->morphToEmotion(e, 260);

    // Servo gesture intensity-mapped
    if      (intensity >= 88) _servo->performGesture(GESTURE_WIGGLE);
    else if (intensity >= 70) _servo->performGesture(random(0,2) == 0 ? GESTURE_NOD : GESTURE_CURIOUS);
    else if (intensity >= 50) _servo->performGesture(random(0,2) == 0 ? GESTURE_TILT_LEFT : GESTURE_TILT_RIGHT);
    else                      _servo->performGesture(GESTURE_PURR); // soft sway for gentle emotions
}

// ─── AI callbacks ─────────────────────────────────────────────────────────────
void SoulEngine::onAIResponseReceived(RobotEmotion e, int intensity, const String& text) {
    triggerEmotion(e, intensity, 5800);
    _lastInteraction = millis();
    _state = STATE_AWAKE_IDLE;  // return to idle — emotion timer handles face
}

void SoulEngine::setAIThinking(bool thinking) {
    if (thinking) {
        _state   = STATE_AI_THINKING;
        _emotion = EMOTION_CURIOUS_SCAN;
        _disp->morphToEmotion(EMOTION_CURIOUS_SCAN, 180);
        _servo->performGesture(GESTURE_CURIOUS);
        // Multi-chirp "thinking" sound
        _audio->playChirp(700, 1100, 100);
        vTaskDelay(pdMS_TO_TICKS(50));
        _audio->playChirp(1100, 1400, 80);
    } else {
        _state = STATE_AWAKE_IDLE;
    }
}

// ─── Touch handlers ───────────────────────────────────────────────────────────
void SoulEngine::onTouchShort() {
    _affection       = min(100, _affection + 7);
    _hunger          = min(100, _hunger + 4);
    _lastInteraction = millis();

    // Response varies by affection level
    if (_affection >= 85) {
        triggerEmotion(EMOTION_KAWAII_KISS, 85, 3800);
        _servo->performGesture(GESTURE_PURR);
        _audio->playHD(voice_kiss_data, sizeof(voice_kiss_data), 4, "Mwah! <3");
    } else {
        triggerEmotion(EMOTION_LOVE, 78, 3500);
        _servo->performGesture(GESTURE_PURR);
        _audio->playHD(voice_love_data, sizeof(voice_love_data), 3, "I Love You! <3");
    }
}

void SoulEngine::onTouchSustained() {
    _affection       = min(100, _affection + 14);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_KAWAII_CAT, 75, 4000);
    _servo->performGesture(GESTURE_CURIOUS);
    // Alternate responses
    const char* responses[] = {"Nya! Meow~", "Purrrr~", "More pets please!", "*melts*"};
    _audio->playHD(voice_cat_data, sizeof(voice_cat_data), 4, responses[random(0, 4)]);
}

void SoulEngine::onTouchOverpet() {
    _affection = max(0, _affection - 5); // too much is too much!
    triggerEmotion(EMOTION_HYPNO_DIZZY, 90, 3800);
    _servo->performGesture(GESTURE_SHAKE);
    _audio->playHD(voice_dizzy_data, sizeof(voice_dizzy_data), 1, "Dizzy! @_@ Too much!");
}

void SoulEngine::onDoubleClap() {
    if (millis() < _clapCoolUntil) return;
    _clapCoolUntil   = millis() + 7000;
    _lastInteraction = millis();
    _affection       = min(100, _affection + 5);
    triggerEmotion(EMOTION_PARTY_DJ, 100, 6500);
    _servo->performGesture(GESTURE_WIGGLE);
    _audio->playHD(voice_party_data, sizeof(voice_party_data), 1, "LET'S GOOO! PARTY!");
}
