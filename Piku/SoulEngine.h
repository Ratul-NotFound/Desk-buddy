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

#define QUIRK_COUNT 10

class SoulEngine {
public:
    void init(DisplayEngine* disp, AudioEngine* audio, ServoEngine* servo);
    void update();   // call every 33ms from Core 1

    // Touch event hooks (called from SensorEngine callbacks)
    void onTouchShort();
    void onTouchSustained();
    void onTouchOverpet();
    void onDoubleClap();

    // BrainEngine hooks
    void triggerEmotion(RobotEmotion e, int intensity, int durationMs = 4000);
    void onAIResponseReceived(RobotEmotion e, int intensity, const String& text);
    void setAIThinking(bool thinking);

    // Autonomous AI request queue (BrainEngine polls this)
    bool   hasPendingAIRequest() const { return _pendingAIRequest; }
    String consumeAIRequest()          { _pendingAIRequest = false; return _pendingAITopic; }

    // Settings
    void setAutoTalkInterval(int minutes) {
        _autoTalkIntervalMs = (unsigned long)minutes * 60000UL;
        _nextAutoTalk = millis() + _autoTalkIntervalMs;
    }
    int  getAutoTalkIntervalMinutes() const { return (int)(_autoTalkIntervalMs / 60000UL); }

    // State accessors
    CompanionState getState()   const { return _state; }
    RobotEmotion   getEmotion() const { return _emotion; }
    int  getAffection()         const { return _affection; }
    int  getEnergy()            const { return _energy; }
    int  getHunger()            const { return _hunger; }
    void feedSnack()    { _hunger = min(100, _hunger + 28); _affection = min(100, _affection + 6); _lastInteraction = millis(); }
    void setState(CompanionState s) { _state = s; }

    // Gaze output (read by DisplayEngine via setGaze)
    float gazeX = 0.0f, gazeY = 0.0f;

private:
    DisplayEngine* _disp;
    AudioEngine*   _audio;
    ServoEngine*   _servo;

    CompanionState _state  = STATE_AWAKE_IDLE;
    RobotEmotion   _emotion = EMOTION_IDLE;

    int  _affection = 85;
    int  _energy    = 100;
    int  _hunger    = 90;

    unsigned long _lastInteraction   = 0;
    unsigned long _emotionResetTime  = 0;
    unsigned long _nextMetabolism    = 0;
    unsigned long _nextGazeShift     = 0;
    unsigned long _nextQuirk         = 0;
    unsigned long _nextAutoTalk      = 0;
    unsigned long _clapCoolUntil     = 0;

    float _targetGazeX = 0.0f;
    float _targetGazeY = 0.0f;

    // Auto-talk
    unsigned long _autoTalkIntervalMs = 10UL * 60000UL;
    bool   _pendingAIRequest  = false;
    String _pendingAITopic;
    int    _topicIndex  = 0;
    int    _quirkIndex  = 0;

    void   _doGazeUpdate();
    void   _doEmotionResetCheck();
    void   _doMetabolismTick();
    void   _doRandomQuirks();
    void   _doAutoTalkCheck();
    void   _fallAsleep();
    void   _wakeUp();
    String _pickTopic();
};
