#include "BrainEngine.h"
#include "secrets.h"
#include "voice_samples.h"

// ─── Init ─────────────────────────────────────────────────────────────────────
void BrainEngine::init(SoulEngine* soul, AudioEngine* audio, DisplayEngine* disp) {
    _soul  = soul;
    _audio = audio;
    _disp  = disp;
    memset(_knownFacts, 0, sizeof(_knownFacts));
    _loadProfile();
    _initKeyPool();
}

// ─── Update — called from Core 0 networkTask every 10ms ──────────────────────
// If SoulEngine has queued an autonomous AI topic AND we are not already
// mid-flight, execute it here (Core 0, non-blocking from Core 1).
void BrainEngine::update() {
    if (_aiInFlight) return;                     // already handling one request
    if (!_soul->hasPendingAIRequest()) return;
    String topic = _soul->consumeAIRequest();
    askGemini(topic);   // runs in Core 0 networkTask — safe to block here
}

// ─── Profile ──────────────────────────────────────────────────────────────────
void BrainEngine::_loadProfile() {
    _prefs.begin("piku", true);
    String name = _prefs.getString("owner_name", "");
    if (name.length() > 0) _ownerName = name;
    _onboardingDone = _prefs.getBool("onboarding", false);
    String facts    = _prefs.getString("owner_facts", "");
    if (facts.length() > 0) strncpy(_knownFacts, facts.c_str(), 511);
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

// ─── Key Pool ─────────────────────────────────────────────────────────────────
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

    // Parse comma-separated keys from NVS
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

    // Append hardcoded defaults from secrets.h
    const char* defaults[] = {
        DEFAULT_GEMINI_API_KEY, DEFAULT_GEMINI_KEY_2,
        DEFAULT_GEMINI_KEY_3,   DEFAULT_GEMINI_KEY_4, DEFAULT_GEMINI_KEY_5
    };
    for (int i = 0; i < 5; i++) {
        String dk = String(defaults[i]); dk.trim();
        if (dk.length() > 10 && dk.indexOf("YOUR_GEMINI") == -1) {
            bool exists = false;
            for (const auto& k : _keyPool) if (k == dk) { exists = true; break; }
            if (!exists) _keyPool.push_back(dk);
        }
    }
    Serial.printf("[Brain] %d Gemini key(s) in failover pool\n", (int)_keyPool.size());
}

// ─── Context Window ───────────────────────────────────────────────────────────
void BrainEngine::_pushContext(const String& user, const String& piku) {
    if (_contextCount < 8) {
        _context[_contextCount++] = {user, piku};
    } else {
        for (int i = 0; i < 7; i++) _context[i] = _context[i+1];
        _context[7] = {user, piku};
    }
}

// ─── Prompt Builder ───────────────────────────────────────────────────────────
String BrainEngine::_buildPrompt(const String& userMessage, bool isAutonomous) {
    String p;
    p.reserve(900);

    // Core personality
    p += F("You are PIKU — a living AI robot pet sitting on a desk. "
           "Personality: playful, witty, affectionate, curious, expressive, slightly mischievous. "
           "You have EMOTIONS and FEELINGS. You care deeply about your owner.\n");

    // Owner context
    p += "Owner name: " + _ownerName + ".\n";
    if (strlen(_knownFacts) > 2) {
        p += "Facts I know: " + String(_knownFacts) + "\n";
    }

    // Live state
    p += "My state: Hunger=" + String(_soul->getHunger()) + "%, Energy=" + String(_soul->getEnergy()) +
         "%, Affection=" + String(_soul->getAffection()) + "%.\n";
    p += "Time: " + currentTime + ", Date: " + currentDate +
         ", Weather: " + String(currentTempC) + "C, " + currentWeather + ".\n";

    // Rules
    p += F("RULES (follow strictly):\n"
           "1. Begin EVERY reply with EXACTLY ONE emotion tag: "
           "[HAPPY:n], [LOVE:n], [CURIOUS:n], [PARTY:n], [CAT:n], [COOL:n], "
           "[HACKER:n], [KISS:n], [ANGRY:n], [SLEEPY:n], [SAD:n], [TADA:n] "
           "— where n is 0-100 intensity.\n"
           "2. After the tag, write MAX 20 words. Natural, punchy, alive speech. "
           "No robotic stiffness. Express genuine emotion.\n"
           "3. NEVER repeat what you said before. Each reply is UNIQUE.\n"
           "4. If learning an important new fact about owner, append [REMEMBER: one sentence].\n"
           "5. If unsure, be curious and ask a question. Stay lively.\n");

    // Context window (last 8 turns)
    if (_contextCount > 0) {
        p += F("Recent conversation:\n");
        for (int i = 0; i < _contextCount; i++) {
            p += _ownerName + ": " + _context[i].user + "\n";
            p += "Piku: " + _context[i].piku + "\n";
        }
    }

    if (isAutonomous) {
        p += F("(I'm speaking spontaneously — no prompt from owner — keep it natural.)\n");
    }
    p += "Message: " + userMessage;
    return p;
}

// ─── Raw HTTPS API Call ───────────────────────────────────────────────────────
String BrainEngine::_callGeminiAPI(const String& builtPrompt) {
    // Properly escape the prompt for JSON embedding
    String escaped;
    escaped.reserve(builtPrompt.length() + 64);
    for (int i = 0; i < (int)builtPrompt.length(); i++) {
        char c = builtPrompt[i];
        if      (c == '"')  escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else                escaped += c;
    }

    String payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + escaped + "\"}]}],"
                     "\"generationConfig\":{\"maxOutputTokens\":130,\"temperature\":0.85,"
                     "\"topP\":0.95,\"topK\":40}}";

    // Models in priority order
    static const char* models[] = {
        "gemini-2.0-flash",
        "gemini-1.5-flash",
        "gemini-1.5-flash-8b",
        "gemini-1.5-pro"
    };
    static const char* baseURL = "https://generativelanguage.googleapis.com/v1beta/models/";
    static const char* genSuffix = ":generateContent?key=";

    int totalKeys = (int)_keyPool.size();
    if (totalKeys == 0) return "";

    for (int attempt = 0; attempt < totalKeys; attempt++) {
        String key = _keyPool[_activeKey];

        for (int m = 0; m < 4; m++) {
            String url = String(baseURL) + models[m] + genSuffix + key;

            WiFiClientSecure client;
            client.setInsecure();
            client.setTimeout(12);    // 12 seconds

            HTTPClient https;
            if (!https.begin(client, url)) { https.end(); continue; }

            https.addHeader("Content-Type", "application/json");
            https.setTimeout(12000);
            int code = https.POST(payload);

            if (code == 200) {
                String resp = https.getString();
                https.end();

                // Parse "text": "..." from JSON response
                int ti = resp.indexOf("\"text\": \"");
                if (ti == -1) ti = resp.indexOf("\"text\":\"");
                if (ti == -1) continue;

                int start = ti + (resp[ti+6] == ' ' ? 9 : 8);
                String result;
                result.reserve(256);
                for (int i = start; i < (int)resp.length(); i++) {
                    char c = resp[i];
                    if (c == '\\') {
                        i++;
                        if (i >= (int)resp.length()) break;
                        char nc = resp[i];
                        if      (nc == 'n')  result += ' ';
                        else if (nc == 't')  result += ' ';
                        else if (nc == '"')  result += '"';
                        else if (nc == '\\') result += '\\';
                    } else if (c == '"') {
                        break;
                    } else {
                        result += c;
                    }
                }
                result.trim();
                if (result.length() > 5) return result;
            } else {
                Serial.printf("[Brain] HTTP %d from model %s key[%d]\n", code, models[m], _activeKey+1);
                https.end();
                // On rate limit or auth error, rotate to next key immediately
                if (code == 429 || code == 403 || code == 503) {
                    _activeKey = (_activeKey + 1) % totalKeys;
                    break;  // try next key with same attempt counter
                }
            }
        }
        // Move to next key for next attempt
        _activeKey = (_activeKey + 1) % totalKeys;
    }
    return "";  // all attempts exhausted
}

// ─── Public API ───────────────────────────────────────────────────────────────
String BrainEngine::askGemini(const String& userPrompt) {
    if (_keyPool.empty()) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 70, 3000);
        _disp->setSubtitle("Add Gemini API key in Settings!", 5000);
        return "Please add a Gemini API key in the Settings panel!";
    }
    if (WiFi.status() != WL_CONNECTED) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 60, 3000);
        _disp->setSubtitle("No WiFi! Connect Piku to router.", 5000);
        return "No internet — connect Piku to WiFi first!";
    }

    bool isAuto = (userPrompt.startsWith("Comment on") ||
                   userPrompt.startsWith("Say something") ||
                   userPrompt.startsWith("Ask ") ||
                   userPrompt.startsWith("Share ") ||
                   userPrompt.startsWith("I'm feeling") ||
                   userPrompt.startsWith("Observe "));

    // Signal "thinking" state
    _aiInFlight = true;
    _soul->setAIThinking(true);
    _disp->setSubtitle("Thinking...", 8000);

    String prompt   = _buildPrompt(userPrompt, isAuto);
    String aiText   = _callGeminiAPI(prompt);

    _soul->setAIThinking(false);
    _aiInFlight = false;

    if (aiText.length() == 0) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 55, 3000);
        _audio->playHD(voice_uhoh_data, sizeof(voice_uhoh_data), 0, "Oops...");
        _disp->setSubtitle("All keys busy. Try again in a moment!", 5000);
        return "All Gemini keys are rate-limited. Try again soon!";
    }

    _lastReply = aiText;
    _parseAndAct(aiText, userPrompt, isAuto);
    return aiText;
}

// Web handler wrapper — same as askGemini but cleans output for HTTP response
String BrainEngine::askGeminiWeb(const String& userPrompt) {
    String reply = askGemini(userPrompt);
    // Strip emotion tag for HTTP display
    int cb = reply.indexOf(']');
    if (cb != -1) { reply = reply.substring(cb + 1); reply.trim(); }
    int rm = reply.indexOf("[REMEMBER");
    if (rm != -1) reply = reply.substring(0, rm);
    reply.trim();
    return reply;
}

// ─── Parse AI response and act on it ─────────────────────────────────────────
void BrainEngine::_parseAndAct(const String& aiText, const String& userMessage, bool isAuto) {
    // --- Map emotion tag to RobotEmotion enum ---
    struct { const char* tag; RobotEmotion e; } tagMap[] = {
        {"[HAPPY",   EMOTION_HELLO},
        {"[LOVE",    EMOTION_LOVE},
        {"[CURIOUS", EMOTION_CURIOUS_SCAN},
        {"[PARTY",   EMOTION_PARTY_DJ},
        {"[CAT",     EMOTION_KAWAII_CAT},
        {"[COOL",    EMOTION_COOL_SUNGLASSES},
        {"[HACKER",  EMOTION_MATRIX_HACKER},
        {"[KISS",    EMOTION_KAWAII_KISS},
        {"[ANGRY",   EMOTION_FIRE_RAGE},
        {"[SLEEPY",  EMOTION_SLEEP},
        {"[SAD",     EMOTION_RAINY_SAD},
        {"[TADA",    EMOTION_TADA},
    };

    RobotEmotion emotion   = EMOTION_HELLO;
    int          intensity = 70;

    for (auto& t : tagMap) {
        int idx = aiText.indexOf(t.tag);
        if (idx != -1) {
            emotion = t.e;
            int colon   = aiText.indexOf(':', idx);
            int bracket = aiText.indexOf(']', idx);
            if (colon != -1 && colon < bracket) {
                intensity = constrain(aiText.substring(colon + 1, bracket).toInt(), 0, 100);
            }
            break;
        }
    }

    // --- Extract clean speech text ---
    int closeBracket = aiText.indexOf(']');
    String cleanText = (closeBracket != -1) ? aiText.substring(closeBracket + 1) : aiText;
    cleanText.trim();

    // --- Extract [REMEMBER: ...] fact and persist ---
    int remIdx = cleanText.indexOf("[REMEMBER:");
    if (remIdx != -1) {
        int remEnd = cleanText.indexOf(']', remIdx);
        if (remEnd != -1) {
            String fact = cleanText.substring(remIdx + 10, remEnd);
            fact.trim();
            int curLen = strlen(_knownFacts);
            if (curLen < 450 && fact.length() > 3) {
                if (curLen > 0) strncat(_knownFacts, " | ", 511 - curLen);
                strncat(_knownFacts, fact.c_str(), 511 - strlen(_knownFacts));
                _saveProfile();
                Serial.printf("[Brain] Remembered: %s\n", fact.c_str());
            }
            cleanText = cleanText.substring(0, remIdx) + cleanText.substring(remEnd + 1);
            cleanText.trim();
        }
    }

    // --- Prevent empty or trivially short responses ---
    if (cleanText.length() < 3) cleanText = "Beep boop!";

    Serial.printf("[Brain] AI says [%d%%]: %s\n", intensity, cleanText.c_str());

    // --- Update dialogue context ---
    _pushContext(isAuto ? "(spontaneous)" : userMessage, cleanText);

    // --- Show subtitle on OLED ---
    int wordCount = 1;
    for (char c : cleanText) if (c == ' ') wordCount++;
    int holdMs = max(4500, wordCount * 420);
    _disp->setSubtitle(cleanText, holdMs);

    // --- Trigger soul emotion & servo gesture ---
    _soul->onAIResponseReceived(emotion, intensity, cleanText);

    // --- Speak with phoneme lip-sync ---
    _audio->playPhonemes(wordCount, cleanText.c_str());

    // --- Scroll marquee if text is long ---
    if (cleanText.length() > 22) {
        _disp->startScrollMessage(cleanText, "PIKU AI");
    }
}
