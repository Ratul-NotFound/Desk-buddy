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
