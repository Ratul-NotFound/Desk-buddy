#include "DisplayEngine.h"

void DisplayEngine::init() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);
    _disp = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (_disp.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        _ready = true;
        _disp.clearDisplay();
        _disp.setTextSize(2);
        _disp.setTextColor(SSD1306_WHITE);
        _disp.setCursor(24, 20);
        _disp.print(F("PIKU AI"));
        _disp.setTextSize(1);
        _disp.setCursor(14, 44);
        _disp.print(F("WAKING UP..."));
        _disp.display();
    }
    _current = _targetShapeFor(EMOTION_IDLE);
    _from    = _current;
    _to      = _current;
    _nextBlink = millis() + random(3000, 6000);
}

void DisplayEngine::morphToEmotion(RobotEmotion e, int durationMs) {
    if (_currentEmotion == e) return;
    _currentEmotion = e;
    _from           = _current;
    _to             = _targetShapeFor(e);
    _morphStart     = millis();
    _morphDuration  = durationMs;
}

EyeShape DisplayEngine::_lerpShape(const EyeShape& a, const EyeShape& b, float t) {
    EyeShape s;
    s.wL       = _lerp(a.wL, b.wL, t);
    s.hL       = _lerp(a.hL, b.hL, t);
    s.rL       = _lerp(a.rL, b.rL, t);
    s.wR       = _lerp(a.wR, b.wR, t);
    s.hR       = _lerp(a.hR, b.hR, t);
    s.rR       = _lerp(a.rR, b.rR, t);
    s.pupilSize = _lerp(a.pupilSize, b.pupilSize, t);
    s.offsetY  = _lerp(a.offsetY, b.offsetY, t);
    s.winkLeft = b.winkLeft;
    return s;
}

// Target EyeShape for each emotion
EyeShape DisplayEngine::_targetShapeFor(RobotEmotion e) {
    // All values: {wL, hL, rL, wR, hR, rR, pupilSize, offsetY, winkLeft}
    switch (e) {
        case EMOTION_IDLE:             return {36, 40, 10, 36, 40, 10, 6, 0,    false};
        case EMOTION_HELLO:            return {36, 44, 10, 36, 44, 10, 7, -4,   false};
        case EMOTION_LOVE:             return {32, 32, 16, 32, 32, 16, 0, 0,    false}; // hearts rendered in overlay
        case EMOTION_TADA:             return {38, 46, 10, 38, 46, 10, 8, -6,   false};
        case EMOTION_PARTY_DJ:         return {36, 36, 4,  36, 36, 4,  0, 0,    false}; // EQ bars overlay
        case EMOTION_CURIOUS_SCAN:     return {30, 42, 8,  30, 42, 8,  5, -3,   false};
        case EMOTION_UHOH_ALERT:       return {28, 28, 6,  28, 28, 6,  4, 2,    false};
        case EMOTION_COOL_SUNGLASSES:  return {44, 32, 6,  44, 32, 6,  0, 0,    false};
        case EMOTION_SLEEP:            return {36, 4,  2,  36, 4,  2,  0, 4,    false};
        case EMOTION_GAMER_PACMAN:     return {28, 28, 14, 28, 28, 14, 5, 0,    false};
        case EMOTION_RAINY_SAD:        return {32, 14, 4,  32, 14, 4,  3, 4,    false};
        case EMOTION_FIRE_RAGE:        return {36, 32, 2,  36, 32, 2,  4, 0,    false};
        case EMOTION_HYPNO_DIZZY:      return {32, 32, 16, 32, 32, 16, 8, 0,    false};
        case EMOTION_KAWAII_CAT:       return {28, 20, 6,  28, 20, 6,  3, 2,    false};
        case EMOTION_JACKPOT_MONEY:    return {36, 36, 4,  36, 36, 4,  0, 0,    false};
        case EMOTION_MATRIX_HACKER:    return {30, 28, 2,  30, 28, 2,  3, 0,    false};
        case EMOTION_KAWAII_KISS:      return {28, 28, 14, 36, 40, 10, 6, 0,    true };
        case EMOTION_FOCUS_STUDY:      return {36, 28, 2,  36, 28, 2,  4, 0,    false};
        case EMOTION_MAGIC_8BALL:      return {28, 36, 8,  28, 36, 8,  5, 0,    false};
        case EMOTION_CLOCK_DISPLAY:    return {24, 16, 4,  24, 16, 4,  3, -8,   false};
        case EMOTION_WEATHER_DISPLAY:  return {24, 16, 4,  24, 16, 4,  3, -8,   false};
        case EMOTION_SENTRY_ALERT:     return {36, 36, 2,  36, 36, 2,  5, 0,    false};
        case EMOTION_SNACK_EAT:        return {32, 36, 6,  32, 36, 6,  6, 0,    false};
        case EMOTION_RPS_SHOW:         return {28, 28, 8,  28, 28, 8,  4, 0,    false};
        default:                       return {36, 40, 10, 36, 40, 10, 6, 0,    false};
    }
}

void DisplayEngine::triggerBlink() {
    _blinking  = true;
    _blinkProg = 0.0f;
}

void DisplayEngine::update(AudioEngine* audio) {
    if (!_ready) return;
    _overlayFrame = millis();

    // Advance morph
    unsigned long now = millis();
    float t = 1.0f;
    if (_morphDuration > 0) {
        t = (float)(now - _morphStart) / (float)_morphDuration;
        t = constrain(t, 0.0f, 1.0f);
        // Ease in-out cubic
        t = t < 0.5f ? 4*t*t*t : 1 - pow(-2*t + 2, 3) / 2;
    }
    _current = _lerpShape(_from, _to, t);

    // Advance blink
    float openRatio = 1.0f;
    if (_blinking) {
        _blinkProg += 0.25f;
        if (_blinkProg >= 1.0f) { _blinking = false; _blinkProg = 0; _nextBlink = now + random(3000, 8000); }
        openRatio = 1.0f - sinf(_blinkProg * M_PI);
    } else if (now >= _nextBlink) {
        triggerBlink();
    }

    if (_scrolling) { _updateScroll(); return; }

    // Handle audio mouth sync
    int mouth = audio ? audio->currentMouthShape : -1;

    // Check for special full-screen emotions
    bool special = (_currentEmotion == EMOTION_PARTY_DJ ||
                    _currentEmotion == EMOTION_SENTRY_ALERT ||
                    _currentEmotion == EMOTION_GAMER_PACMAN ||
                    _currentEmotion == EMOTION_MATRIX_HACKER ||
                    _currentEmotion == EMOTION_CURIOUS_SCAN ||
                    _currentEmotion == EMOTION_CLOCK_DISPLAY ||
                    _currentEmotion == EMOTION_WEATHER_DISPLAY);

    _disp.clearDisplay();

    if (mouth >= 0) {
        // During audio: show speech face
        _renderSpeechFace(mouth, nullptr);
    } else if (special) {
        _renderSpecial();
    } else {
        _renderIdle(_gazeX, _gazeY, openRatio);
        _drawOverlay(_currentEmotion);
    }
    _disp.display();
}

void DisplayEngine::_renderIdle(float gx, float gy, float openRatio) {
    EyeShape s = _current;
    float hL = s.hL * openRatio;
    float hR = (s.winkLeft ? 2.0f : s.hR * openRatio);
    if (hL < 2) hL = 2;
    if (hR < 2) hR = 2;

    int lx = EYE_L_CX + (int)gx;
    int rx = EYE_R_CX + (int)gx;
    int cy = EYE_CY   + (int)(gy + s.offsetY);

    _disp.fillRoundRect(lx - (int)(s.wL/2), cy - (int)(hL/2), (int)s.wL, (int)hL, (int)s.rL, SSD1306_WHITE);
    _disp.fillRoundRect(rx - (int)(s.wR/2), cy - (int)(hR/2), (int)s.wR, (int)hR, (int)s.rR, SSD1306_WHITE);

    if (openRatio > 0.45f && s.pupilSize > 0) {
        int px = (int)(gx * 0.45f);
        int py = (int)(gy * 0.45f);
        int pr = (int)s.pupilSize;
        _disp.fillCircle(lx + px, cy + py, pr, SSD1306_BLACK);
        _disp.fillCircle(rx + px, cy + py, pr, SSD1306_BLACK);
        _disp.fillCircle(lx + px + 2, cy + py - 2, max(1, pr/3), SSD1306_WHITE);
        _disp.fillCircle(rx + px + 2, cy + py - 2, max(1, pr/3), SSD1306_WHITE);
    }
}

void DisplayEngine::_drawHeart(int cx, int cy, int size) {
    int r = size / 4;
    _disp.fillCircle(cx - r, cy - r, r, SSD1306_WHITE);
    _disp.fillCircle(cx + r, cy - r, r, SSD1306_WHITE);
    _disp.fillTriangle(cx - size/2, cy - r + 1, cx + size/2, cy - r + 1, cx, cy + size/2, SSD1306_WHITE);
}

void DisplayEngine::_drawOverlay(RobotEmotion e) {
    int f = (int)(_overlayFrame / 80) % 8;
    switch (e) {
        case EMOTION_LOVE:
        case EMOTION_KAWAII_KISS:
            _drawHeart(EYE_L_CX, EYE_CY, 30);
            _drawHeart(EYE_R_CX, EYE_CY, 30);
            break;
        case EMOTION_KAWAII_CAT:
            // Cat ears
            _disp.fillTriangle(EYE_L_CX - 16, EYE_CY - 16, EYE_L_CX, EYE_CY - 16,
                               EYE_L_CX - 8, EYE_CY - 30, SSD1306_WHITE);
            _disp.fillTriangle(EYE_R_CX, EYE_CY - 16, EYE_R_CX + 16, EYE_CY - 16,
                               EYE_R_CX + 8, EYE_CY - 30, SSD1306_WHITE);
            // Blush
            for (int i = -3; i <= 3; i += 2) {
                _disp.drawPixel(EYE_L_CX - 12 + i, EYE_CY + 12, SSD1306_WHITE);
                _disp.drawPixel(EYE_R_CX + 10 + i, EYE_CY + 12, SSD1306_WHITE);
            }
            break;
        case EMOTION_SLEEP:
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            if (f >= 0) { _disp.setCursor(80, 26); _disp.print(F("z")); }
            if (f >= 3) { _disp.setCursor(92, 16); _disp.print(F("Z")); }
            if (f >= 6) { _disp.setCursor(106, 6); _disp.print(F("Z")); }
            break;
        case EMOTION_RAINY_SAD:
            for (int i = 0; i < 6; i++) {
                int rx = 12 + i * 20;
                int ry = ((int)(_overlayFrame / 5) + i * 14) % 40 + 10;
                _disp.drawLine(rx, ry, rx - 2, ry + 6, SSD1306_WHITE);
            }
            break;
        case EMOTION_FIRE_RAGE: {
            int fs = (int)(_overlayFrame / 40) % 12;
            for (int i = 0; i < 8; i++) {
                int fx = 12 + i * 14;
                int fh = 6 + (fs + i * 3) % 10;
                _disp.fillTriangle(fx - 4, 16, fx + 4, 16, fx, 16 - fh, SSD1306_WHITE);
            }
            break;
        }
        default: break;
    }
}

void DisplayEngine::_drawMouth(int shape) {
    switch (shape) {
        case 0: _disp.drawFastHLine(56, 50, 16, SSD1306_WHITE); break;
        case 1: _disp.drawCircle(64, 50, 4, SSD1306_WHITE); break;
        case 2: _disp.fillRoundRect(56, 46, 16, 8, 3, SSD1306_WHITE);
                _disp.fillRoundRect(58, 47, 12, 6, 2, SSD1306_BLACK); break;
        case 3: _disp.fillRoundRect(52, 44, 24, 12, 5, SSD1306_WHITE);
                _disp.fillCircle(64, 46, 5, SSD1306_BLACK); break;
        default: _disp.drawCircle(64, 48, 7, SSD1306_WHITE);
                 _disp.fillRect(52, 41, 24, 7, SSD1306_BLACK); break;
    }
}

void DisplayEngine::_renderSpeechFace(int mouthShape, const char* subtitle) {
    EyeShape neutral = _targetShapeFor(EMOTION_HELLO);
    _renderIdle(0, 0, 1.0f);
    _drawMouth(mouthShape);
    if (subtitle) {
        _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
        int len = strlen(subtitle);
        int sx  = (128 - len * 6) / 2;
        _disp.setCursor(max(2, sx), 57);
        _disp.print(subtitle);
    }
}

void DisplayEngine::_renderSpecial() {
    int f = (int)(_overlayFrame / 30) % 64;
    switch (_currentEmotion) {
        case EMOTION_PARTY_DJ: {
            int b1 = 8 + f % 24, b2 = 8 + (f + 12) % 24;
            _disp.fillRoundRect(EYE_L_CX - 16, EYE_CY - b1/2, 32, b1, 4, SSD1306_WHITE);
            _disp.fillRoundRect(EYE_R_CX - 16, EYE_CY - b2/2, 32, b2, 4, SSD1306_WHITE);
            _disp.fillRect(64 - f%20/2, 54, f%20, 4, SSD1306_WHITE);
            break;
        }
        case EMOTION_SENTRY_ALERT:
            if ((f / 8) % 2 == 0) { _disp.fillRect(0,0,128,64,SSD1306_WHITE); _disp.setTextColor(SSD1306_BLACK); }
            else _disp.setTextColor(SSD1306_WHITE);
            _disp.setTextSize(2); _disp.setCursor(8, 14); _disp.print(F("SENTRY"));
            _disp.setTextSize(1); _disp.setCursor(8, 42); _disp.print(F("INTRUDER DETECTED!"));
            break;
        case EMOTION_MATRIX_HACKER:
            for (int col = 6; col < 124; col += 12) {
                int sy = (f * 3 + col * 7) % 50;
                _disp.drawFastVLine(col, sy, 8, SSD1306_WHITE);
            }
            _disp.drawRect(34, 14, 60, 36, SSD1306_WHITE);
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(22, 55); _disp.print(F("[ ACCESS GRANTED ]"));
            break;
        case EMOTION_CURIOUS_SCAN: {
            int sy = 12 + f % 40;
            _disp.drawRoundRect(8, 8, 112, 48, 6, SSD1306_WHITE);
            _disp.drawFastHLine(4, 32, 120, SSD1306_WHITE);
            _disp.drawFastVLine(64, 4, 56, SSD1306_WHITE);
            _disp.drawFastHLine(12, sy, 104, SSD1306_WHITE);
            _disp.fillCircle(64, sy, 3, SSD1306_WHITE);
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(14, 54); _disp.print(F("AI THINKING..."));
            break;
        }
        default: _renderIdle(_gazeX, _gazeY, 1.0f); break;
    }
}

void DisplayEngine::_updateScroll() {
    unsigned long now = millis();
    if (now < _nextScrollTick) { _disp.display(); return; }
    _nextScrollTick = now + 25;
    _scrollX -= 3;
    int totalLen = (int)(_scrollText.length() * 6);
    if (_scrollX < -totalLen) { _scrolling = false; return; }

    _disp.clearDisplay();
    _disp.fillRoundRect(EYE_L_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);
    if (_scrollTitle.length() > 0) {
        _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
        _disp.setCursor(4, 26); _disp.print(_scrollTitle);
    }
    _disp.drawRoundRect(2, 36, 124, 26, 4, SSD1306_WHITE);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(_scrollX, 46); _disp.print(_scrollText);
    _disp.display();
}

void DisplayEngine::startScrollMessage(const String& text, const char* title) {
    _scrollText  = text;
    _scrollTitle = (title) ? String(title) : "";
    _scrollX     = 128;
    _scrolling   = true;
    _nextScrollTick = millis();
}

void DisplayEngine::showVolumeHUD(int vol, bool muted) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(38, 10); _disp.print(F("VOLUME"));
    int fillW = muted ? 0 : (vol * 96) / 100;
    _disp.drawRoundRect(16, 28, 96, 16, 4, SSD1306_WHITE);
    if (fillW > 0) _disp.fillRoundRect(16, 28, fillW, 16, 4, SSD1306_WHITE);
    _disp.setCursor(30, 50);
    if (muted || vol == 0) _disp.print(F("MUTED"));
    else { _disp.print(vol); _disp.print(F("%")); }
    _disp.display();
}

// Mini-game + special screen renderers
void DisplayEngine::renderFlappyGame(int bY, float vel, int score, int hi, int pX, int pGY, bool over) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.fillCircle(24, bY, 6, SSD1306_WHITE);
    _disp.fillCircle(26, bY - 2, 2, SSD1306_BLACK);
    _disp.fillTriangle(30, bY - 1, 35, bY + 1, 30, bY + 3, SSD1306_WHITE);
    _disp.fillRect(pX, 0, 16, pGY, SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY - 4, 20, 4, SSD1306_WHITE);
    _disp.fillRect(pX, pGY + 28, 16, 64 - (pGY + 28), SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY + 28, 20, 4, SSD1306_WHITE);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(4, 2); _disp.print(F("SCORE:")); _disp.print(score);
    _disp.setCursor(76, 2); _disp.print(F("HI:")); _disp.print(hi);
    if (over) {
        _disp.fillRoundRect(16, 20, 96, 26, 4, SSD1306_BLACK);
        _disp.drawRoundRect(16, 20, 96, 26, 4, SSD1306_WHITE);
        _disp.setCursor(24, 25); _disp.print(F("GAME OVER!"));
        _disp.setCursor(20, 35); _disp.print(F("Tap to Retry"));
    }
    _disp.display();
}

void DisplayEngine::renderRPS(const char* choice) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(14, 4); _disp.print(F("ROCK PAPER SCISSORS"));
    _disp.drawRoundRect(24, 18, 80, 42, 6, SSD1306_WHITE);
    _disp.setTextSize(2);
    _disp.setCursor(64 - strlen(choice) * 6, 30);
    _disp.print(choice);
    _disp.display();
}

void DisplayEngine::renderSentryAlert(int tick) {
    _currentEmotion = EMOTION_SENTRY_ALERT;
    update(nullptr);
}

void DisplayEngine::renderSnackEat(int frame) {
    if (!_ready) return;
    _disp.clearDisplay();
    int h = (frame % 2 == 0) ? 36 : 14;
    _disp.fillRoundRect(EYE_L_CX - 16, EYE_CY - h/2, 32, h, 6, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 16, EYE_CY - h/2, 32, h, 6, SSD1306_WHITE);
    _disp.fillCircle(64, 50, (frame%2==0)?10:3, SSD1306_WHITE);
    _disp.display();
}

void DisplayEngine::renderMagic8Ball(const char* answer) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.drawCircle(64, 30, 24, SSD1306_WHITE);
    _disp.fillCircle(64, 30, 14, SSD1306_WHITE);
    _disp.fillTriangle(54, 36, 74, 36, 64, 20, SSD1306_BLACK);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    int len = strlen(answer);
    _disp.setCursor(max(4, (128 - len*6)/2), 58);
    _disp.print(answer);
    _disp.display();
}

void DisplayEngine::renderClockScreen(const char* timeStr, const char* dateStr) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.fillRoundRect(EYE_L_CX - 12, 4, 24, 16, 4, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 12, 4, 24, 16, 4, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);
    _disp.drawRoundRect(4, 24, 120, 38, 5, SSD1306_WHITE);
    _disp.setTextSize(2); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(64 - strlen(timeStr)*6, 28); _disp.print(timeStr);
    _disp.setTextSize(1);
    _disp.setCursor(64 - strlen(dateStr)*3, 48); _disp.print(dateStr);
    _disp.display();
}

void DisplayEngine::renderWeatherScreen(int tempC, int humidity, const char* condition) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.drawRoundRect(2, 2, 124, 60, 6, SSD1306_WHITE);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(8, 8); _disp.print(F("LIVE WEATHER"));
    _disp.setTextSize(3); _disp.setCursor(10, 22);
    _disp.print(tempC); _disp.setTextSize(1); _disp.print(F("C"));
    _disp.setCursor(80, 24); _disp.print(F("HUM:"));
    _disp.setCursor(80, 34); _disp.print(humidity); _disp.print(F("%"));
    _disp.setCursor(8, 48); _disp.print(condition);
    _disp.display();
}
