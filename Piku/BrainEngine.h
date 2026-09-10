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

class BrainEngine {
public:
    void init(SoulEngine* soul, AudioEngine* audio, DisplayEngine* disp);
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
    SoulEngine*    _soul;
    AudioEngine*   _audio;
    DisplayEngine* _disp;
    Preferences    _prefs;

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
