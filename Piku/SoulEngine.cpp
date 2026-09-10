#include "SoulEngine.h"

void SoulEngine::init(DisplayEngine* disp, AudioEngine* audio, ServoEngine* servo) {
    _disp  = disp;
    _audio = audio;
    _servo = servo;
    _lastInteraction = millis();
    _nextAutoTalk    = millis() + _autoTalkIntervalMs;
    _nextMetabolism  = millis() + 45000;
    _nextGazeShift   = millis() + random(1800, 4200);
    _nextQuirk       = millis() + random(30000, 60000);
}

void SoulEngine::update() {
    _doGazeUpdate();
    _doEmotionResetCheck();
    _doMetabolismTick();
    _doRandomQuirks();
    _doAutoTalkCheck();
    _disp->setGaze(gazeX, gazeY);
}

void SoulEngine::_doGazeUpdate() {
    unsigned long now = millis();
    if (now >= _nextGazeShift) {
        _nextGazeShift = now + random(1800, 4500);
        _targetGazeX = random(-8, 9);
        _targetGazeY = random(-4, 5);
        if (_state == STATE_AWAKE_IDLE && _emotion == EMOTION_IDLE && !_servo->isBusy()) {
            // Natural micro head tracking following eyes
            _servo->setTarget(SERVO_CENTER + _targetGazeX * 1.6f);
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
        if (!_servo->isBusy()) _servo->setTarget(SERVO_CENTER);
        _disp->morphToEmotion(EMOTION_IDLE, 400);
    }
}

void SoulEngine::_doRandomQuirks() {
    if (_state != STATE_AWAKE_IDLE || _emotion != EMOTION_IDLE) return;
    unsigned long now = millis();
    if (now < _nextQuirk) return;
    _nextQuirk = now + random(35000, 75000);

    int quirk = random(0, 4);
    switch (quirk) {
        case 0:
            // Inquisitive chirp + curious head tilt
            _audio->playChirp(750, 1350, 110);
            _servo->performGesture(GESTURE_CURIOUS);
            break;
        case 1:
            // Playful nod + happy double-chirp
            _audio->playChirp(900, 1400, 70);
            _disp->triggerBlink();
            _servo->performGesture(GESTURE_NOD);
            break;
        case 2:
            // Subtle wink + soft chirp
            _disp->triggerBlink();
            _audio->playChirp(1100, 1600, 60);
            break;
        case 3:
            // Look around
            _targetGazeX = random(0, 2) == 0 ? 8 : -8;
            _targetGazeY = random(-3, 4);
            _servo->performGesture(random(0, 2) == 0 ? GESTURE_TILT_LEFT : GESTURE_TILT_RIGHT);
            break;
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
            _audio->playChirp(500, 250, 250); // stomach growl chirp
        }
        if (_energy < 10) {
            _state  = STATE_DEEP_SLEEP;
            _emotion = EMOTION_SLEEP;
            _disp->morphToEmotion(EMOTION_SLEEP, 600);
            _servo->setTarget(SERVO_CENTER - 18.0f);
            _audio->playHD(voice_sleep_data, sizeof(voice_sleep_data), 0, "Zzz...");
        }
    } else if (_state == STATE_DEEP_SLEEP) {
        _energy = min(100, _energy + 5);
        if (_energy >= 80) {
            _state  = STATE_AWAKE_IDLE;
            _emotion = EMOTION_IDLE;
            _disp->morphToEmotion(EMOTION_HELLO, 400);
            _servo->performGesture(GESTURE_NOD);
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
        "Comment on the current time of day in a funny witty way.",
        "Say something about the weather right now.",
        "Ask the owner how they are doing today.",
        "Share one short interesting fun fact about technology or animals.",
        "Say something playful and random to cheer the owner up.",
        "Ask what the owner is working on right now.",
        "Say something witty about being a cute living desk robot.",
    };
    return topics[random(0, 7)];
}

void SoulEngine::triggerEmotion(RobotEmotion e, int intensity, int durationMs) {
    _emotion = e;
    _emotionResetTime = millis() + durationMs;
    _disp->morphToEmotion(e, 300);
    if (intensity >= 80) _servo->performGesture(GESTURE_WIGGLE);
    else if (intensity >= 60) _servo->performGesture(random(0,2)==0 ? GESTURE_NOD : GESTURE_CURIOUS);
    else if (intensity >= 40) _servo->performGesture(random(0,2)==0 ? GESTURE_TILT_LEFT : GESTURE_TILT_RIGHT);
}

void SoulEngine::onAIResponseReceived(RobotEmotion e, int intensity, const String& text) {
    _state = STATE_AI_SPEAKING;
    triggerEmotion(e, intensity, 5000);
    _lastInteraction = millis();
    _disp->startScrollMessage(text, "PIKU AI");
}

void SoulEngine::setAIThinking(bool thinking) {
    if (thinking) {
        _state  = STATE_AI_THINKING;
        _emotion = EMOTION_CURIOUS_SCAN;
        _disp->morphToEmotion(EMOTION_CURIOUS_SCAN, 200);
        _servo->performGesture(GESTURE_CURIOUS);
        _audio->playChirp(700, 1300, 160);
    } else {
        _state  = STATE_AWAKE_IDLE;
    }
}

void SoulEngine::onTouchShort() {
    _affection = min(100, _affection + 5);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_LOVE, 70, 3500);
    _servo->performGesture(GESTURE_NOD);
    _audio->playHD(voice_love_data, sizeof(voice_love_data), 3, "I Love You! <3");
}

void SoulEngine::onTouchSustained() {
    _affection = min(100, _affection + 10);
    _lastInteraction = millis();
    triggerEmotion(EMOTION_KAWAII_CAT, 65, 3500);
    _servo->performGesture(GESTURE_CURIOUS);
    _audio->playHD(voice_cat_data, sizeof(voice_cat_data), 4, "Nya! Meow!");
}

void SoulEngine::onTouchOverpet() {
    triggerEmotion(EMOTION_HYPNO_DIZZY, 85, 3500);
    _servo->performGesture(GESTURE_SHAKE);
    _audio->playHD(voice_dizzy_data, sizeof(voice_dizzy_data), 1, "Dizzy! @__@");
}

void SoulEngine::onDoubleClap() {
    if (millis() < _clapCoolUntil) return;
    _clapCoolUntil = millis() + 5000;
    _lastInteraction = millis();
    triggerEmotion(EMOTION_PARTY_DJ, 100, 6000);
    _servo->performGesture(GESTURE_WIGGLE);
    _audio->playHD(voice_party_data, sizeof(voice_party_data), 1, "PARTY TIME!");
}
