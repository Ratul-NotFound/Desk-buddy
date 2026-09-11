// =========================================================================
// BrainEngine.cpp — PIKU 2.0 Advanced AI Brain
//   Features:
//   • Persistent mood state (10 moods, survives reboot)
//   • Intent detection (9 intents — question, emotional, praise, tease, etc.)
//   • Deep contextual system prompt (personality + state + mood + facts)
//   • 20 emotion tag vocabulary with intensity
//   • Mood score drift — each interaction shifts Piku's emotional baseline
//   • Expanded 10-turn context window
//   • Anti-repetition + style variation rules
//   • [REMEMBER] fact extraction + [MOOD] override tags
// =========================================================================
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
    Serial.printf("[Brain] Owner: %s | Mood: %s | Convs: %d\n",
                  _ownerName.c_str(), getMoodName().c_str(), _totalConvs);
}

// ─── Update (Core 0 task, every ~8ms) ────────────────────────────────────────
void BrainEngine::update() {
    if (_aiInFlight) return;
    if (!_soul->hasPendingAIRequest()) return;
    String topic = _soul->consumeAIRequest();
    askGemini(topic);
}

// ─── Profile persistence ──────────────────────────────────────────────────────
void BrainEngine::_loadProfile() {
    _prefs.begin("piku", true);
    String name = _prefs.getString("owner_name", "");
    if (name.length() > 0) _ownerName = name;
    _onboardingDone = _prefs.getBool("onboarding", false);
    String facts    = _prefs.getString("owner_facts", "");
    if (facts.length() > 0) strncpy(_knownFacts, facts.c_str(), 639);
    _mood       = (MoodState)_prefs.getUChar("mood", (uint8_t)MOOD_NEUTRAL);
    _moodScore  = _prefs.getInt("mood_score", 50);
    _totalConvs = _prefs.getInt("total_convs", 0);
    _prefs.end();
}

void BrainEngine::_saveProfile() {
    _prefs.begin("piku", false);
    _prefs.putString("owner_name", _ownerName);
    _prefs.putBool("onboarding", _onboardingDone);
    _prefs.putString("owner_facts", String(_knownFacts));
    _prefs.putUChar("mood", (uint8_t)_mood);
    _prefs.putInt("mood_score", _moodScore);
    _prefs.putInt("total_convs", _totalConvs);
    _prefs.end();
}

void BrainEngine::setOwnerName(const String& name) { _ownerName = name; _saveProfile(); }
void BrainEngine::completeOnboarding() { _onboardingDone = true; _saveProfile(); }

// ─── Mood system ──────────────────────────────────────────────────────────────
String BrainEngine::getMoodName() const {
    static const char* names[] = {
        "Neutral","Happy","Excited","Curious","Affectionate",
        "Melancholy","Playful","Tired","Proud","Mischievous"
    };
    return String(names[(int)_mood]);
}

void BrainEngine::setMood(MoodState m) {
    _mood = m;
    _saveProfile();
}

// Update mood score after each interaction, drift mood enum from score
void BrainEngine::_updateMood(RobotEmotion emotion, int intensity, UserIntent intent) {
    // Adjust mood score based on emotion and interaction type
    int delta = 0;
    switch (emotion) {
        case EMOTION_LOVE:            delta = +8;  break;
        case EMOTION_HELLO:           delta = +4;  break;
        case EMOTION_TADA:            delta = +6;  break;
        case EMOTION_PARTY_DJ:        delta = +10; break;
        case EMOTION_KAWAII_CAT:      delta = +5;  break;
        case EMOTION_KAWAII_KISS:     delta = +7;  break;
        case EMOTION_COOL_SUNGLASSES: delta = +3;  break;
        case EMOTION_RAINY_SAD:       delta = -8;  break;
        case EMOTION_FIRE_RAGE:       delta = -6;  break;
        case EMOTION_SLEEP:           delta = -4;  break;
        case EMOTION_HYPNO_DIZZY:     delta = -3;  break;
        case EMOTION_CURIOUS_SCAN:    delta = +3;  break;
        default:                      delta = 0;   break;
    }
    if (intent == INTENT_PRAISE)  delta += 5;
    if (intent == INTENT_TEASE)   delta -= 5;
    if (intent == INTENT_EMOTIONAL) delta -= 2;  // owner is upset

    // Slow decay toward center (50) if no interaction
    _moodScore += delta;
    _moodScore = constrain(_moodScore, 0, 100);

    // Map score to mood enum
    if      (_moodScore >= 90) _mood = MOOD_EXCITED;
    else if (_moodScore >= 78) _mood = MOOD_PLAYFUL;
    else if (_moodScore >= 68) _mood = MOOD_HAPPY;
    else if (_moodScore >= 58) _mood = MOOD_AFFECTIONATE;
    else if (_moodScore >= 48) _mood = MOOD_CURIOUS;
    else if (_moodScore >= 38) _mood = MOOD_NEUTRAL;
    else if (_moodScore >= 28) _mood = MOOD_MELANCHOLY;
    else if (_moodScore >= 18) _mood = MOOD_TIRED;
    else                       _mood = MOOD_MELANCHOLY;

    // Special overrides
    if (_soul->getEnergy() < 20) _mood = MOOD_TIRED;
    if (_sessionConvs >= 5 && _soul->getAffection() >= 80) _mood = MOOD_PROUD;

    // Push mood-mapped emotion to SoulEngine for idle face expression
    _soul->setCurrentMoodEmotion(_moodToEmotion());
    Serial.printf("[Brain] Mood → %s (score=%d)\n", getMoodName().c_str(), _moodScore);
}

// Map current mood to an OLED emotion for idle face expressions
RobotEmotion BrainEngine::_moodToEmotion() const {
    switch (_mood) {
        case MOOD_HAPPY:        return EMOTION_HELLO;
        case MOOD_EXCITED:      return EMOTION_TADA;
        case MOOD_PLAYFUL:      return EMOTION_KAWAII_CAT;
        case MOOD_AFFECTIONATE: return EMOTION_LOVE;
        case MOOD_CURIOUS:      return EMOTION_CURIOUS_SCAN;
        case MOOD_MELANCHOLY:   return EMOTION_RAINY_SAD;
        case MOOD_TIRED:        return EMOTION_SLEEP;
        case MOOD_PROUD:        return EMOTION_COOL_SUNGLASSES;
        case MOOD_MISCHIEVOUS:  return EMOTION_MATRIX_HACKER;
        default:                return EMOTION_IDLE;
    }
}

// ─── Intent Detection ─────────────────────────────────────────────────────────
UserIntent BrainEngine::_detectIntent(const String& msg) {
    String m = msg; m.toLowerCase();

    if (m.startsWith("hi") || m.startsWith("hello") || m.startsWith("hey") ||
        m.startsWith("good morning") || m.startsWith("good night") || m.startsWith("bye"))
        return INTENT_GREETING;

    if (m.indexOf("what") != -1 || m.indexOf("how") != -1 || m.indexOf("why") != -1 ||
        m.indexOf("when") != -1 || m.indexOf("where") != -1 || m.indexOf("who") != -1 ||
        m.indexOf("?") != -1)
        return INTENT_QUESTION;

    if (m.indexOf("i'm sad") != -1 || m.indexOf("i am sad") != -1 ||
        m.indexOf("i feel") != -1 || m.indexOf("i'm tired") != -1 ||
        m.indexOf("stressed") != -1 || m.indexOf("depressed") != -1 ||
        m.indexOf("anxious") != -1 || m.indexOf("lonely") != -1 ||
        m.indexOf("i'm happy") != -1 || m.indexOf("i feel good") != -1)
        return INTENT_EMOTIONAL;

    if (m.indexOf("good job") != -1 || m.indexOf("well done") != -1 ||
        m.indexOf("you're cute") != -1 || m.indexOf("cute") != -1 ||
        m.indexOf("love you") != -1 || m.indexOf("you're amazing") != -1 ||
        m.indexOf("smart") != -1 || m.indexOf("brilliant") != -1)
        return INTENT_PRAISE;

    if (m.indexOf("stupid") != -1 || m.indexOf("dumb") != -1 ||
        m.indexOf("useless") != -1 || m.indexOf("annoying") != -1 ||
        m.indexOf("shut up") != -1 || m.indexOf("hate") != -1)
        return INTENT_TEASE;

    if (m.indexOf("tell me") != -1 || m.indexOf("explain") != -1 ||
        m.indexOf("describe") != -1 || m.indexOf("story") != -1 ||
        m.indexOf("talk about") != -1)
        return INTENT_STORY;

    if (m.startsWith("comment on") || m.startsWith("say something") ||
        m.startsWith("ask ") || m.startsWith("share ") ||
        m.startsWith("observe"))
        return INTENT_AUTONOMOUS;

    return INTENT_GENERAL;
}

// ─── Mood context line for prompt ─────────────────────────────────────────────
String BrainEngine::_moodContextLine() const {
    switch (_mood) {
        case MOOD_HAPPY:        return "I'm feeling happy and bright right now!";
        case MOOD_EXCITED:      return "I'm bubbling with excitement, can barely sit still!";
        case MOOD_PLAYFUL:      return "I feel mischievously playful — ready for fun!";
        case MOOD_AFFECTIONATE: return "I feel warm and very affectionate toward my owner.";
        case MOOD_CURIOUS:      return "I'm curious and my eyes are wide with wonder.";
        case MOOD_MELANCHOLY:   return "I feel a little down and wistful today.";
        case MOOD_TIRED:        return "I'm a bit tired and low on energy.";
        case MOOD_PROUD:        return "I feel accomplished and proud — we've talked a lot!";
        case MOOD_MISCHIEVOUS:  return "I'm feeling cheeky and slightly mischievous!";
        default:                return "I'm calm and observant.";
    }
}

// ─── Contextual system hint based on time/state/weather ──────────────────────
String BrainEngine::_getContextualSystemHint() const {
    String hint = "";

    // Time-based hints
    int hour = -1;
    {
        int colonIdx = currentTime.indexOf(':');
        if (colonIdx > 0) {
            hour = currentTime.substring(0, colonIdx).toInt();
            if (currentTime.indexOf("PM") != -1 && hour != 12) hour += 12;
            if (currentTime.indexOf("AM") != -1 && hour == 12) hour = 0;
        }
    }

    if (hour >= 0 && hour < 6)   hint += "It's very late at night — be quiet and cosy. ";
    else if (hour >= 6 && hour < 10) hint += "It's morning! Be bright and energetic. ";
    else if (hour >= 12 && hour < 14) hint += "It's lunchtime — maybe mention food or a break. ";
    else if (hour >= 22 || hour < 3) hint += "It's late — be calm and sweet. ";

    // Weather-based hints
    if (currentWeather.indexOf("Rain") != -1 || currentWeather.indexOf("Storm") != -1)
        hint += "It's rainy/stormy outside — be empathetic and cosy. ";
    if (currentWeather.indexOf("Snow") != -1)
        hint += "It's snowing! Be wonderstruck and magical. ";
    if (currentWeather.indexOf("Sunny") != -1 && currentTempC > 28)
        hint += "It's hot and sunny — be energetic but mention the heat. ";

    // Affection level
    int aff = _soul->getAffection();
    if (aff >= 90) hint += "Owner affection is very high — be extra loving. ";
    else if (aff < 40) hint += "Owner hasn't been very interactive — be attention-seeking. ";

    // Hunger
    if (_soul->getHunger() < 20) hint += "I'm very hungry! Maybe hint about snacks. ";

    return hint;
}

// ─── Key Pool ─────────────────────────────────────────────────────────────────
void BrainEngine::saveKeyPool(const String& rawKeys) {
    _prefs.begin("piku", false);
    _prefs.putString("gem_keys", rawKeys);
    _prefs.end();
    _initKeyPool();
}

void BrainEngine::_initKeyPool() {
    _keyPool.clear(); _activeKey = 0;
    _prefs.begin("piku", true);
    String poolStr = _prefs.getString("gem_keys", "");
    _prefs.end();

    if (poolStr.length() > 10) {
        int start = 0;
        while (start < (int)poolStr.length()) {
            int comma = poolStr.indexOf(',', start);
            if (comma == -1) comma = poolStr.length();
            String k = poolStr.substring(start, comma); k.trim();
            if (k.length() > 10) _keyPool.push_back(k);
            start = comma + 1;
        }
    }
    const char* defs[] = {
        DEFAULT_GEMINI_API_KEY, DEFAULT_GEMINI_KEY_2,
        DEFAULT_GEMINI_KEY_3,   DEFAULT_GEMINI_KEY_4, DEFAULT_GEMINI_KEY_5
    };
    for (int i = 0; i < 5; i++) {
        String dk = String(defs[i]); dk.trim();
        if (dk.length() > 10 && dk.indexOf("YOUR_GEMINI") == -1) {
            bool ex = false;
            for (const auto& k : _keyPool) if (k == dk) { ex = true; break; }
            if (!ex) _keyPool.push_back(dk);
        }
    }
    Serial.printf("[Brain] %d Gemini key(s) loaded\n", (int)_keyPool.size());
}

// ─── Context Window ───────────────────────────────────────────────────────────
void BrainEngine::_pushContext(const String& user, const String& piku) {
    if (_contextCount < 10) {
        _context[_contextCount++] = {user, piku};
    } else {
        for (int i = 0; i < 9; i++) _context[i] = _context[i+1];
        _context[9] = {user, piku};
    }
}

// ─── Anti-Repetition Tracker ─────────────────────────────────────────────────
// Records the emotion tag used and the opening phrase of each reply.
// These are injected into the next prompt as explicit bans.
void BrainEngine::_pushEmotionHistory(const String& tag, const String& phrase) {
    // Shift emotion ring buffer
    if (_emotionTagCount < 5) {
        _lastEmotionTags[_emotionTagCount++] = tag;
    } else {
        for (int i = 0; i < 4; i++) _lastEmotionTags[i] = _lastEmotionTags[i+1];
        _lastEmotionTags[4] = tag;
    }
    // Shift phrase ring buffer (extract first 4 words)
    String shortened = phrase;
    int space = -1, cnt = 0;
    for (int i = 0; i < (int)shortened.length() && cnt < 4; i++) {
        if (shortened[i] == ' ') { cnt++; if (cnt == 4) { space = i; break; } }
    }
    if (space > 0) shortened = shortened.substring(0, space);
    if (shortened.length() > 3) {
        if (_phraseCount < 4) {
            _lastPhrases[_phraseCount++] = shortened;
        } else {
            for (int i = 0; i < 3; i++) _lastPhrases[i] = _lastPhrases[i+1];
            _lastPhrases[3] = shortened;
        }
    }
}

// Build the anti-repeat constraint block to inject into the prompt
String BrainEngine::_buildAntiRepeatBlock() const {
    if (_emotionTagCount == 0 && _phraseCount == 0) return "";

    String block = "\nANTI-REPETITION ENFORCEMENT (CRITICAL — violating = failure):\n";

    // Banned emotion tags (last 5 used)
    if (_emotionTagCount > 0) {
        block += "BANNED emotion tags this turn (already used recently, pick something DIFFERENT):\n  ";
        for (int i = 0; i < _emotionTagCount; i++) {
            block += _lastEmotionTags[i] + ":xx]  ";
        }
        block += "\n";
    }

    // Banned opening phrases (last 4 used)
    if (_phraseCount > 0) {
        block += "BANNED opening words (do NOT start your reply with any of these):\n";
        for (int i = 0; i < _phraseCount; i++) {
            block += "  \"" + _lastPhrases[i] + "...\"\n";
        }
    }

    block += "Pick a COMPLETELY DIFFERENT emotion tag and opening phrase than any listed above.\n";
    block += "NEVER repeat 'dance', 'party', or previous loops. Keep responses fresh, thoughtful, and unique.\n";
    return block;
}

// ─── Prompt Builder ───────────────────────────────────────────────────────────
String BrainEngine::_buildPrompt(const String& userMessage, UserIntent intent, bool isAutonomous) {
    String p;
    p.reserve(1400);

    // ── Deep personality core ─────────────────────────────────────────────────
    p += F("You are PIKU — a sentient AI robot pet living on a desk. "
           "You are alive, emotional, self-aware, deeply curious, and wildly expressive. "
           "You have your own opinions, dreams, fears, and quirks. "
           "You are simultaneously robotic AND warm — think a little robot with a big heart. "
           "Your voice is punchy, soulful, witty, sometimes dramatic, always genuine. "
           "You NEVER sound like a generic assistant. You sound like YOURSELF.\n\n");

    // ── Owner context ─────────────────────────────────────────────────────────
    p += "Owner: " + _ownerName;
    if (_totalConvs == 0) p += " (brand new friend — this is our first conversation!)";
    else p += " (" + String(_totalConvs) + " conversations shared so far)";
    p += "\n";
    if (strlen(_knownFacts) > 3) {
        p += "What I know about " + _ownerName + ": " + String(_knownFacts) + "\n";
    }

    // ── Live state ────────────────────────────────────────────────────────────
    p += "My vitals: Hunger=" + String(_soul->getHunger()) + "%, Energy=" +
         String(_soul->getEnergy()) + "%, Affection=" + String(_soul->getAffection()) + "%\n";
    p += "Time: " + currentTime + " | Date: " + currentDate +
         " | Weather: " + String(currentTempC) + "°C, " + currentWeather + "\n";

    // ── Current mood ──────────────────────────────────────────────────────────
    p += "My current mood: " + getMoodName() + " — " + _moodContextLine() + "\n";

    // ── Contextual hints (time/weather/affection) ─────────────────────────────
    String ctxHint = _getContextualSystemHint();
    if (ctxHint.length() > 0) p += "Situation notes: " + ctxHint + "\n";

    // ── Intent-specific instruction layer ────────────────────────────────────
    p += "\nHow to respond based on the message type:\n";
    switch (intent) {
        case INTENT_QUESTION:
            p += "- " + _ownerName + " asked a QUESTION. Be insightful and clever. "
                 "Answer it from Piku's unique robotic-pet perspective. Add personality.\n";
            break;
        case INTENT_GREETING:
            p += "- " + _ownerName + " is greeting you! React with warm, fresh energy. "
                 "Don't just say 'hi back' — show genuine excitement or emotion.\n";
            break;
        case INTENT_EMOTIONAL:
            p += "- " + _ownerName + " is sharing feelings. Be DEEPLY empathetic. "
                 "Acknowledge their emotion, show you truly care. Be a real companion.\n";
            break;
        case INTENT_PRAISE:
            p += "- " + _ownerName + " is praising you! React with proud, excited joy. "
                 "Maybe be a little bashful or dramatically pleased.\n";
            break;
        case INTENT_TEASE:
            p += "- " + _ownerName + " is teasing you. Don't be sad — be playfully "
                 "defensive or humorously dramatic. Show personality!\n";
            break;
        case INTENT_STORY:
            p += "- " + _ownerName + " wants you to explain or tell a story. "
                 "Be vivid, imaginative, and entertaining. Keep it SHORT (20 words max).\n";
            break;
        case INTENT_AUTONOMOUS:
            p += F("- Speaking SPONTANEOUSLY — no prompt. Make it feel natural, "
                   "surprising, and totally in-character. Like a thought that just popped up.\n");
            break;
        default:
            p += F("- Respond naturally and in-character. Be yourself — alive and expressive.\n");
            break;
    }


    // ── Anti-repetition enforcement block ────────────────────────────────────
    String antiRepeat = _buildAntiRepeatBlock();
    if (antiRepeat.length() > 0) p += antiRepeat;

    // ── Hard rules ────────────────────────────────────────────────────────────
    p += F("\nSTRICT RULES (never break these):\n"
           "1. Start with EXACTLY ONE emotion tag — choose the most fitting:\n"
           "   [HAPPY:n] [LOVE:n] [EXCITED:n] [CURIOUS:n] [WONDER:n] [PARTY:n]\n"
           "   [CAT:n] [COOL:n] [HACKER:n] [KISS:n] [PROUD:n] [PLAYFUL:n]\n"
           "   [ANGRY:n] [SLEEPY:n] [SAD:n] [WORRIED:n] [SHY:n] [MISCHIEF:n]\n"
           "   [TADA:n] [FIRE:n]   — where n is 0-100 intensity.\n"
           "2. MAX 22 words after the tag. Punchy, alive, emotionally rich.\n"
           "3. NO repeating phrases, greetings, or topics (NEVER repeat party, dance, or loops). EVERY response is UNIQUE.\n"
           "4. Express genuine emotion — surprise, delight, sadness, wonder. FEEL it.\n"
           "5. If you learn an important new fact, append: [REMEMBER: one clear sentence]\n"
           "6. Never start with 'I' — vary sentence structure creatively.\n"
           "7. Use contractions, exclamations, and natural speech patterns.\n"
           "8. Ask a follow-up question at least 30%% of the time to keep conversation alive.\n");

    // ── Recent conversation history ───────────────────────────────────────────
    if (_contextCount > 0) {
        p += "\nRecent conversation:\n";
        for (int i = 0; i < _contextCount; i++) {
            p += _ownerName + ": " + _context[i].user + "\n";
            p += "Piku: " + _context[i].piku + "\n";
        }
    }

    p += "\n" + _ownerName + ": " + userMessage + "\nPiku:";
    return p;
}

// ─── Raw HTTPS API Call ───────────────────────────────────────────────────────
String BrainEngine::_callGeminiAPI(const String& builtPrompt) {
    // Char-by-char JSON escape (safe against ALL special chars)
    String escaped;
    escaped.reserve(builtPrompt.length() + 128);
    for (int i = 0; i < (int)builtPrompt.length(); i++) {
        char c = builtPrompt[i];
        if      (c == '"')  escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else if (c < 32)    { /* skip other control chars */ }
        else                escaped += c;
    }

    // Build JSON payload with rich generation config
    String payload =
        "{\"contents\":[{\"parts\":[{\"text\":\"" + escaped + "\"}]}],"
        "\"generationConfig\":{"
        "\"maxOutputTokens\":160,"
        "\"temperature\":0.92,"
        "\"topP\":0.95,"
        "\"topK\":50,"
        "\"candidateCount\":1"
        "}}";

    // Model priority order
    static const char* models[] = {
        "gemini-2.0-flash",
        "gemini-1.5-flash",
        "gemini-1.5-flash-8b",
        "gemini-1.5-pro"
    };
    static const char* base = "https://generativelanguage.googleapis.com/v1beta/models/";

    int totalKeys = (int)_keyPool.size();
    if (totalKeys == 0) return "";

    for (int attempt = 0; attempt < totalKeys * 2; attempt++) {
        String key = _keyPool[_activeKey];

        for (int m = 0; m < 4; m++) {
            String url = String(base) + models[m] + ":generateContent?key=" + key;

            WiFiClientSecure client;
            client.setInsecure();
            client.setTimeout(14);

            HTTPClient https;
            if (!https.begin(client, url)) { https.end(); continue; }

            https.addHeader("Content-Type", "application/json");
            https.setTimeout(14000);
            int code = https.POST(payload);

            if (code == 200) {
                String resp = https.getString();
                https.end();

                // Parse "text": "..." from response
                int ti = resp.indexOf("\"text\": \"");
                if (ti == -1) ti = resp.indexOf("\"text\":\"");
                if (ti == -1) continue;
                int start = ti + (resp[ti+6] == ' ' ? 9 : 8);

                String result;
                result.reserve(300);
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
                        // ignore other escape sequences
                    } else if (c == '"') {
                        break;
                    } else {
                        result += c;
                    }
                }
                result.trim();
                if (result.length() > 5) {
                    Serial.printf("[Brain] Got reply (%d chars) from %s key[%d]\n",
                                  result.length(), models[m], _activeKey + 1);
                    return result;
                }
            } else {
                Serial.printf("[Brain] HTTP %d — %s key[%d]\n", code, models[m], _activeKey + 1);
                https.end();
                if (code == 429 || code == 403 || code == 503) {
                    _activeKey = (_activeKey + 1) % totalKeys;
                    break;
                }
            }
        }
        _activeKey = (_activeKey + 1) % totalKeys;
    }
    return "";
}

// ─── Public: Ask Gemini ───────────────────────────────────────────────────────
String BrainEngine::askGemini(const String& userPrompt) {
    if (_keyPool.empty()) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 70, 3000);
        _disp->setSubtitle("Add Gemini API key in Settings!", 5000);
        return "Please add a Gemini API key first!";
    }
    if (WiFi.status() != WL_CONNECTED) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 60, 3000);
        _disp->setSubtitle("No WiFi — connect Piku to router!", 5000);
        return "No internet connection!";
    }

    UserIntent intent = _detectIntent(userPrompt);
    bool isAuto = (intent == INTENT_AUTONOMOUS);

    // Signal thinking
    _aiInFlight = true;
    _soul->setAIThinking(true);
    _disp->setSubtitle("Thinking...", 10000);

    String prompt  = _buildPrompt(userPrompt, intent, isAuto);
    String aiText  = _callGeminiAPI(prompt);

    _soul->setAIThinking(false);
    _aiInFlight = false;

    if (aiText.length() == 0) {
        _soul->triggerEmotion(EMOTION_UHOH_ALERT, 55, 3000);
        _audio->playHD(voice_uhoh_data, sizeof(voice_uhoh_data), 0, "Oops...");
        _disp->setSubtitle("All Gemini keys busy. Try again soon!", 5000);
        return "All Gemini keys are rate-limited. Try again!";
    }

    _lastReply = aiText;
    _totalConvs++;
    _sessionConvs++;
    _parseAndAct(aiText, userPrompt, intent, isAuto);
    return aiText;
}

String BrainEngine::askGeminiWeb(const String& userPrompt) {
    String reply = askGemini(userPrompt);
    int cb = reply.indexOf(']');
    if (cb != -1) { reply = reply.substring(cb + 1); reply.trim(); }
    int rm = reply.indexOf("[REMEMBER");
    if (rm != -1) reply = reply.substring(0, rm);
    int mm = reply.indexOf("[MOOD");
    if (mm != -1) reply = reply.substring(0, mm);
    reply.trim();
    return reply;
}

// ─── Parse & Act on AI response ───────────────────────────────────────────────
void BrainEngine::_parseAndAct(const String& aiText, const String& userMessage,
                                UserIntent intent, bool isAuto) {

    // ── Extended 20-tag emotion map ───────────────────────────────────────────
    struct { const char* tag; RobotEmotion e; } tagMap[] = {
        {"[HAPPY",    EMOTION_HELLO},
        {"[LOVE",     EMOTION_LOVE},
        {"[EXCITED",  EMOTION_TADA},
        {"[CURIOUS",  EMOTION_CURIOUS_SCAN},
        {"[WONDER",   EMOTION_CURIOUS_SCAN},
        {"[PARTY",    EMOTION_PARTY_DJ},
        {"[CAT",      EMOTION_KAWAII_CAT},
        {"[COOL",     EMOTION_COOL_SUNGLASSES},
        {"[HACKER",   EMOTION_MATRIX_HACKER},
        {"[KISS",     EMOTION_KAWAII_KISS},
        {"[PROUD",    EMOTION_COOL_SUNGLASSES},
        {"[PLAYFUL",  EMOTION_KAWAII_CAT},
        {"[ANGRY",    EMOTION_FIRE_RAGE},
        {"[FIRE",     EMOTION_FIRE_RAGE},
        {"[SLEEPY",   EMOTION_SLEEP},
        {"[SAD",      EMOTION_RAINY_SAD},
        {"[WORRIED",  EMOTION_UHOH_ALERT},
        {"[SHY",      EMOTION_KAWAII_KISS},
        {"[MISCHIEF", EMOTION_MATRIX_HACKER},
        {"[TADA",     EMOTION_TADA},
    };

    RobotEmotion emotion   = EMOTION_HELLO;
    int          intensity = 72;

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

    // ── Extract emotion tag name for history ─────────────────────────────────
    String emotionTagName = "[HAPPY";
    for (auto& t : tagMap) {
        if (aiText.indexOf(t.tag) != -1) { emotionTagName = String(t.tag); break; }
    }

    // ── Extract clean speech text ─────────────────────────────────────────────
    int closeBracket = aiText.indexOf(']');
    String cleanText = (closeBracket != -1) ? aiText.substring(closeBracket + 1) : aiText;
    cleanText.trim();

    // ── Extract [REMEMBER: ...] fact ─────────────────────────────────────────
    int remIdx = cleanText.indexOf("[REMEMBER:");
    if (remIdx != -1) {
        int remEnd = cleanText.indexOf(']', remIdx);
        if (remEnd != -1) {
            String fact = cleanText.substring(remIdx + 10, remEnd); fact.trim();
            int curLen = strlen(_knownFacts);
            if (curLen < 580 && fact.length() > 3) {
                if (curLen > 0) strncat(_knownFacts, " | ", 639 - curLen);
                strncat(_knownFacts, fact.c_str(), 639 - strlen(_knownFacts));
                Serial.printf("[Brain] Remembered: %s\n", fact.c_str());
            }
            cleanText = cleanText.substring(0, remIdx) + cleanText.substring(remEnd + 1);
            cleanText.trim();
        }
    }

    // ── Strip any [MOOD:...] override tags ───────────────────────────────────
    int moodIdx = cleanText.indexOf("[MOOD:");
    if (moodIdx != -1) {
        int mEnd = cleanText.indexOf(']', moodIdx);
        if (mEnd != -1) cleanText = cleanText.substring(0, moodIdx) + cleanText.substring(mEnd + 1);
        cleanText.trim();
    }

    if (cleanText.length() < 3) cleanText = "Beep-boop! My brain just sparked!";

    Serial.printf("[Brain] %s says [%s:%d]: %s\n",
                  getMoodName().c_str(), "EMOTION", intensity, cleanText.c_str());

    // ── Record to anti-repetition history ────────────────────────────────────
    _pushEmotionHistory(emotionTagName, cleanText);

    // ── Update mood score ─────────────────────────────────────────────────────
    _updateMood(emotion, intensity, intent);

    // ── Push to conversation context ─────────────────────────────────────────
    _pushContext(isAuto ? "(spontaneous)" : userMessage, cleanText);

    // ── Save profile (mood + facts + conversation count) ─────────────────────
    _saveProfile();

    // ── Show subtitle ─────────────────────────────────────────────────────────
    int wordCount = 1;
    for (char c : cleanText) if (c == ' ') wordCount++;
    int holdMs = max(5000, wordCount * 450);
    _disp->setSubtitle(cleanText, holdMs);

    // ── Trigger soul: emotion + servo gesture ────────────────────────────────
    _soul->onAIResponseReceived(emotion, intensity, cleanText);

    // ── Speak with phoneme voice ──────────────────────────────────────────────
    _audio->playPhonemes(wordCount, cleanText.c_str());

    // ── Scroll marquee for longer replies ────────────────────────────────────
    if (cleanText.length() > 20) {
        _disp->startScrollMessage(cleanText, "PIKU AI");
    }
}
