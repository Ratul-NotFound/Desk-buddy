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

void DisplayEngine::setClockWeather(const String& timeStr, const String& dateStr, int tempC, int hum, const String& cond) {
    _timeStr     = timeStr;
    _dateStr     = dateStr;
    _tempC       = tempC;
    _humidity    = hum;
    _weatherCond = cond;
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
        t = t < 0.5f ? 4*t*t*t : 1.0f - pow(-2.0f*t + 2.0f, 3) / 2.0f;
    }
    _current = _lerpShape(_from, _to, t);

    // Advance blink (suppress during sleep)
    float openRatio = 1.0f;
    if (_currentEmotion != EMOTION_SLEEP) {
        if (_blinking) {
            _blinkProg += 0.25f;
            if (_blinkProg >= 1.0f) { _blinking = false; _blinkProg = 0; _nextBlink = now + random(3000, 8000); }
            openRatio = 1.0f - sinf(_blinkProg * M_PI);
        } else if (now >= _nextBlink) {
            triggerBlink();
        }
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
                    _currentEmotion == EMOTION_WEATHER_DISPLAY ||
                    _currentEmotion == EMOTION_JACKPOT_MONEY ||
                    _currentEmotion == EMOTION_FOCUS_STUDY ||
                    _currentEmotion == EMOTION_MAGIC_8BALL ||
                    _currentEmotion == EMOTION_SNACK_EAT ||
                    _currentEmotion == EMOTION_RPS_SHOW);

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

    if (openRatio > 0.45f && s.pupilSize > 0 && _currentEmotion != EMOTION_SLEEP) {
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
            _drawHeart(EYE_L_CX, EYE_CY, 30);
            _drawHeart(EYE_R_CX, EYE_CY, 30);
            break;
        case EMOTION_KAWAII_KISS: {
            _drawHeart(EYE_L_CX, EYE_CY, 26);
            int hx = 64 + ((int)(_overlayFrame / 40) % 50);
            int hy = 44 - ((int)(_overlayFrame / 40) % 32);
            _drawHeart(hx, hy, 12);
            break;
        }
        case EMOTION_KAWAII_CAT: {
            // Cat ears
            int et = (f % 4 < 2) ? 2 : 0;
            _disp.fillTriangle(EYE_L_CX - 16, EYE_CY - 12, EYE_L_CX + 2, EYE_CY - 12, EYE_L_CX - 7, EYE_CY - 28 - et, SSD1306_WHITE);
            _disp.fillTriangle(EYE_R_CX - 2, EYE_CY - 12, EYE_R_CX + 16, EYE_CY - 12, EYE_R_CX + 7, EYE_CY - 28 - et, SSD1306_WHITE);
            // Whisker lines
            _disp.drawLine(8, 22, 18, 24, SSD1306_WHITE);
            _disp.drawLine(8, 28, 18, 26, SSD1306_WHITE);
            _disp.drawLine(120, 22, 110, 24, SSD1306_WHITE);
            _disp.drawLine(120, 28, 110, 26, SSD1306_WHITE);
            // Cute mouth
            _disp.drawCircle(60, 48, 3, SSD1306_WHITE);
            _disp.fillRect(57, 45, 6, 3, SSD1306_BLACK);
            _disp.drawCircle(66, 48, 3, SSD1306_WHITE);
            _disp.fillRect(66, 45, 6, 3, SSD1306_BLACK);
            break;
        }
        case EMOTION_COOL_SUNGLASSES: {
            // Sunglasses frames
            _disp.fillRoundRect(EYE_L_CX - 22, EYE_CY - 14, 44, 28, 5, SSD1306_WHITE);
            _disp.fillRoundRect(EYE_R_CX - 22, EYE_CY - 14, 44, 28, 5, SSD1306_WHITE);
            _disp.fillRect(EYE_L_CX + 18, EYE_CY - 10, 18, 6, SSD1306_WHITE);
            // Glare reflection lines
            int gx = (int)(_overlayFrame / 20) % 30 - 15;
            _disp.drawLine(EYE_L_CX - 12 + gx, EYE_CY + 10, EYE_L_CX - 4 + gx, EYE_CY - 10, SSD1306_BLACK);
            _disp.drawLine(EYE_R_CX - 12 + gx, EYE_CY + 10, EYE_R_CX - 4 + gx, EYE_CY - 10, SSD1306_BLACK);
            // Cool smile
            _disp.drawCircle(64, 46, 8, SSD1306_WHITE);
            _disp.fillRect(52, 40, 24, 8, SSD1306_BLACK);
            break;
        }
        case EMOTION_SLEEP:
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            if (f >= 0) { _disp.setCursor(80, 26); _disp.print(F("z")); }
            if (f >= 3) { _disp.setCursor(92, 16); _disp.print(F("Z")); }
            if (f >= 6) { _disp.setCursor(106, 6); _disp.print(F("Z")); }
            break;
        case EMOTION_RAINY_SAD: {
            // Rain cloud
            _disp.fillCircle(50, 8, 6, SSD1306_WHITE);
            _disp.fillCircle(64, 6, 8, SSD1306_WHITE);
            _disp.fillCircle(78, 8, 6, SSD1306_WHITE);
            _disp.fillRoundRect(42, 8, 44, 6, 3, SSD1306_WHITE);
            int dy = 16 + ((int)(_overlayFrame / 30) % 36);
            _disp.drawLine(48, dy, 48, dy + 3, SSD1306_WHITE);
            _disp.drawLine(64, (dy + 12) % 36 + 16, 64, (dy + 12) % 36 + 19, SSD1306_WHITE);
            _disp.drawLine(80, (dy + 24) % 36 + 16, 80, (dy + 24) % 36 + 27, SSD1306_WHITE);
            // Sad mouth
            _disp.drawCircle(64, 56, 4, SSD1306_WHITE);
            _disp.fillRect(58, 56, 12, 6, SSD1306_BLACK);
            break;
        }
        case EMOTION_FIRE_RAGE: {
            int fs = (int)(_overlayFrame / 40);
            for (int i = 0; i < 3; i++) {
                int fh1 = 6 + (int)(10 * fabs(sin((fs * 0.4) + i)));
                int fh2 = 6 + (int)(10 * fabs(cos((fs * 0.4) + i)));
                _disp.fillTriangle(EYE_L_CX - 12 + i*10, EYE_CY - 16, EYE_L_CX - 8 + i*10, EYE_CY - 16 - fh1, EYE_L_CX - 4 + i*10, EYE_CY - 16, SSD1306_WHITE);
                _disp.fillTriangle(EYE_R_CX - 12 + i*10, EYE_CY - 16, EYE_R_CX - 8 + i*10, EYE_CY - 16 - fh2, EYE_R_CX - 4 + i*10, EYE_CY - 16, SSD1306_WHITE);
            }
            // Clenched teeth mouth
            _disp.drawRect(52, 48, 24, 6, SSD1306_WHITE);
            for (int x = 56; x < 76; x += 4) _disp.drawFastVLine(x, 48, 6, SSD1306_WHITE);
            break;
        }
        case EMOTION_HYPNO_DIZZY: {
            int rot = (int)(_overlayFrame / 30);
            for (int r = 16; r > 3; r -= 5) {
                int offset = (rot + r * 2) % 8;
                _disp.drawRoundRect(EYE_L_CX - r, EYE_CY - r, r*2, r*2, offset, SSD1306_WHITE);
                _disp.drawRoundRect(EYE_R_CX - r, EYE_CY - r, r*2, r*2, offset, SSD1306_WHITE);
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
            _disp.fillRect(64 - (f%20)/2, 54, f%20, 4, SSD1306_WHITE);
            break;
        }
        case EMOTION_SENTRY_ALERT:
            if ((f / 8) % 2 == 0) { _disp.fillRect(0,0,128,64,SSD1306_WHITE); _disp.setTextColor(SSD1306_BLACK); }
            else _disp.setTextColor(SSD1306_WHITE);
            _disp.setTextSize(2); _disp.setCursor(8, 14); _disp.print(F("SENTRY"));
            _disp.setTextSize(1); _disp.setCursor(8, 42); _disp.print(F("INTRUDER DETECTED!"));
            break;
        case EMOTION_GAMER_PACMAN: {
            int px = ((int)(_overlayFrame / 40) * 4) % 140 - 20;
            _disp.fillCircle(px, EYE_CY, 18, SSD1306_WHITE);
            int mouthOpen = ((f / 4) % 2 == 0) ? 14 : 4;
            _disp.fillTriangle(px, EYE_CY, px + 20, EYE_CY - mouthOpen, px + 20, EYE_CY + mouthOpen, SSD1306_BLACK);
            for (int dot = px + 28; dot < 128; dot += 18) {
                _disp.fillCircle(dot, EYE_CY, 3, SSD1306_WHITE);
            }
            int gx = px - 32;
            int gy = EYE_CY - 16;
            _disp.fillRoundRect(gx, gy, 24, 32, 10, SSD1306_WHITE);
            int footShift = ((f / 2) % 2 == 0) ? 0 : 2;
            _disp.fillRect(gx + 2 + footShift, gy + 22, 5, 4, SSD1306_BLACK);
            _disp.fillRect(gx + 11 + footShift, gy + 22, 5, 4, SSD1306_BLACK);
            _disp.fillRect(gx + 19 - footShift, gy + 22, 5, 4, SSD1306_BLACK);
            _disp.fillCircle(gx + 7, gy + 8, 3, SSD1306_BLACK);
            _disp.fillCircle(gx + 17, gy + 8, 3, SSD1306_BLACK);
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(18, 54); _disp.print(F("LEVEL UP! [ 1UP ]"));
            break;
        }
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
        case EMOTION_CLOCK_DISPLAY:
            renderClockScreen(_timeStr.c_str(), _dateStr.c_str());
            break;
        case EMOTION_WEATHER_DISPLAY:
            renderWeatherScreen(_tempC, _humidity, _weatherCond.c_str());
            break;
        case EMOTION_JACKPOT_MONEY: {
            _disp.drawRoundRect(EYE_L_CX - 18, EYE_CY - 18, 36, 36, 4, SSD1306_WHITE);
            _disp.drawRoundRect(EYE_R_CX - 18, EYE_CY - 18, 36, 36, 4, SSD1306_WHITE);
            _disp.setTextSize(2); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(EYE_L_CX - 6, EYE_CY - 7); _disp.print(F("$"));
            _disp.setCursor(EYE_R_CX - 6, EYE_CY - 7); _disp.print(F("$"));
            int cy = 4 + (f * 2 % 54);
            _disp.fillCircle(12, cy, 3, SSD1306_WHITE);
            _disp.fillCircle(116, (cy + 18) % 54 + 4, 3, SSD1306_WHITE);
            _disp.setTextSize(1); _disp.setCursor(24, 54); _disp.print(F("JACKPOT! $$$"));
            break;
        }
        case EMOTION_FOCUS_STUDY: {
            _disp.drawCircle(EYE_L_CX, EYE_CY, 16, SSD1306_WHITE);
            _disp.drawCircle(EYE_R_CX, EYE_CY, 16, SSD1306_WHITE);
            _disp.drawLine(EYE_L_CX + 16, EYE_CY - 4, EYE_R_CX - 16, EYE_CY - 4, SSD1306_WHITE);
            _disp.fillCircle(EYE_L_CX, EYE_CY, 5, SSD1306_WHITE);
            _disp.fillCircle(EYE_R_CX, EYE_CY, 5, SSD1306_WHITE);
            _disp.drawRoundRect(20, 48, 88, 12, 3, SSD1306_WHITE);
            int bar = 4 + (f % 80);
            _disp.fillRect(22, 50, bar, 8, SSD1306_WHITE);
            _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
            _disp.setCursor(24, 2); _disp.print(F("FOCUS MODE [ 25:00 ]"));
            break;
        }
        case EMOTION_MAGIC_8BALL:
            renderMagic8Ball(_magic8Answer.c_str());
            break;
        case EMOTION_SNACK_EAT:
            renderSnackEat(f / 4);
            break;
        case EMOTION_RPS_SHOW:
            renderRPS(_rpsChoice.c_str());
            break;
        default:
            _renderIdle(_gazeX, _gazeY, 1.0f);
            break;
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
    _disp.setCursor(42, 8); _disp.print(F("VOLUME"));
    int fillW = muted ? 0 : (vol * 96) / 100;
    _disp.drawRoundRect(16, 24, 96, 16, 4, SSD1306_WHITE);
    if (fillW > 0) _disp.fillRoundRect(16, 24, fillW, 16, 4, SSD1306_WHITE);
    _disp.setCursor(44, 46);
    if (muted || vol == 0) _disp.print(F("MUTED"));
    else { _disp.print(vol); _disp.print(F("%")); }
    _disp.display();
}

// Mini-game + special screen renderers
void DisplayEngine::renderFlappyGame(int bY, float vel, int score, int hi, int pX, int pGY, bool over) {
    if (!_ready) return;
    _disp.clearDisplay();
    // Bird
    _disp.fillCircle(24, bY, 6, SSD1306_WHITE);
    _disp.fillCircle(26, bY - 2, 2, SSD1306_BLACK);
    _disp.fillTriangle(30, bY - 1, 35, bY + 1, 30, bY + 3, SSD1306_WHITE);
    // Pipes
    _disp.fillRect(pX, 0, 16, pGY, SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY - 4, 20, 4, SSD1306_WHITE);
    _disp.fillRect(pX, pGY + 28, 16, 64 - (pGY + 28), SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY + 28, 20, 4, SSD1306_WHITE);
    // Score HUD
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
    _disp.drawRoundRect(20, 18, 88, 42, 6, SSD1306_WHITE);
    _disp.setTextSize(2);
    int len = strlen(choice);
    _disp.setCursor(64 - len * 6, 30);
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
    _disp.fillCircle(64, 50, (frame % 2 == 0) ? 10 : 3, SSD1306_WHITE);
    _disp.display();
}

void DisplayEngine::renderMagic8Ball(const char* answer) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.drawCircle(64, 26, 22, SSD1306_WHITE);
    _disp.fillCircle(64, 26, 12, SSD1306_WHITE);
    _disp.fillTriangle(56, 31, 72, 31, 64, 18, SSD1306_BLACK);
    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    int len = strlen(answer);
    _disp.setCursor(max(4, (128 - len*6)/2), 54);
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
    int tLen = strlen(timeStr);
    _disp.setCursor(max(8, 64 - tLen*6), 28); _disp.print(timeStr);
    _disp.setTextSize(1);
    int dLen = strlen(dateStr);
    _disp.setCursor(max(8, 64 - dLen*3), 48); _disp.print(dateStr);
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
    _disp.setCursor(76, 24); _disp.print(F("HUM:"));
    _disp.setCursor(76, 34); _disp.print(humidity); _disp.print(F("%"));
    _disp.setCursor(8, 48); _disp.print(condition);
    _disp.display();
}
