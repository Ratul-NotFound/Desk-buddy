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
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=",
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=",
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=",
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-lite-latest:generateContent?key="
    };

    String aiText;
    int totalKeys = (int)_keyPool.size();
    int attempts  = 0;

    while (attempts < totalKeys && aiText.length() == 0) {
        String key = _keyPool[_activeKey];
        for (int m = 0; m < 4 && aiText.length() == 0; m++) {
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
