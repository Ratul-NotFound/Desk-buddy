#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <vector>
#include "SoulEngine.h"
#include "AudioEngine.h"
#include "DisplayEngine.h"

struct ContextTurn { String user, piku; };

// ─── Mood State (persisted to NVS, survives reboots) ──────────────────────────
enum MoodState : uint8_t {
    MOOD_NEUTRAL = 0,
    MOOD_HAPPY,
    MOOD_EXCITED,
    MOOD_CURIOUS,
    MOOD_AFFECTIONATE,
    MOOD_MELANCHOLY,
    MOOD_PLAYFUL,
    MOOD_TIRED,
    MOOD_PROUD,
    MOOD_MISCHIEVOUS
};

// ─── User Intent (detected before each AI call) ───────────────────────────────
enum UserIntent : uint8_t {
    INTENT_QUESTION,     // "what is...", "how do...", "why..."
    INTENT_GREETING,     // "hi", "hello", "hey piku"
    INTENT_COMMAND,      // "do this", "play", "show"
    INTENT_EMOTIONAL,    // "I'm sad", "I'm tired", "I feel..."
    INTENT_STORY,        // "tell me", "explain", "describe"
    INTENT_PRAISE,       // "good job", "you're cute", "well done"
    INTENT_TEASE,        // "you're dumb", "stupid robot"
    INTENT_AUTONOMOUS,   // spontaneous (no user prompt)
    INTENT_GENERAL
};

class BrainEngine {
public:
    void init(SoulEngine* soul, AudioEngine* audio, DisplayEngine* disp);
    void update();    // Core 0 task — polls SoulEngine for pending AI requests

    // Gemini AI
    String askGemini(const String& userPrompt);
    String askGeminiWeb(const String& userPrompt);   // clean version for HTTP
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

    // Mood (persisted)
    MoodState getMood()         const { return _mood; }
    String    getMoodName()     const;
    void      setMood(MoodState m);

    // Weather/time (injected by NetworkEngine)
    int    currentTempC    = 25;
    int    currentHumidity = 65;
    String currentWeather  = "Sunny";
    String currentTime     = "12:00 PM";
    String currentDate     = "Thu, 01 Jan";

    // Conversation stats
    int    getTotalConversations() const { return _totalConvs; }

private:
    SoulEngine*    _soul;
    AudioEngine*   _audio;
    DisplayEngine* _disp;
    Preferences    _prefs;

    // Owner memory
    String _ownerName       = "Friend";
    bool   _onboardingDone  = false;
    char   _knownFacts[640] = {};       // expanded to 640 bytes

    // Key pool
    std::vector<String> _keyPool;
    int _activeKey = 0;

    // Conversation context (10 turns for deeper memory)
    ContextTurn _context[10];
    int _contextCount = 0;

    // Persistent mood & session stats
    MoodState _mood         = MOOD_NEUTRAL;
    int       _moodScore    = 50;         // 0-100, drives mood drift
    int       _totalConvs   = 0;
    int       _sessionConvs = 0;

    // Flags
    bool   _aiInFlight   = false;
    String _lastReply;

    // Anti-repetition: track last 5 emotion tags and last 4 key phrases
    String _lastEmotionTags[5];   // e.g. "[PARTY", "[EXCITED"
    int    _emotionTagCount = 0;
    String _lastPhrases[4];       // first 3 words of last 4 replies
    int    _phraseCount = 0;

    // Private methods
    void   _loadProfile();
    void   _saveProfile();
    void   _initKeyPool();
    String _buildPrompt(const String& userMessage, UserIntent intent, bool isAutonomous);
    String _callGeminiAPI(const String& builtPrompt);
    void   _parseAndAct(const String& aiText, const String& userMessage, UserIntent intent, bool isAuto);
    void   _pushContext(const String& user, const String& piku);
    void   _pushEmotionHistory(const String& tag, const String& phrase);
    String _buildAntiRepeatBlock() const;
    UserIntent _detectIntent(const String& msg);
    void   _updateMood(RobotEmotion emotion, int intensity, UserIntent intent);
    String _moodContextLine() const;
    String _getContextualSystemHint() const;
    RobotEmotion _moodToEmotion() const;
};
