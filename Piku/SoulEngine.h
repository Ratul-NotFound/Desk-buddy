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
