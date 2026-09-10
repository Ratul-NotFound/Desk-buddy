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
        _disp.setCursor(22, 16);
        _disp.print(F("PIKU AI"));
        _disp.setTextSize(1);
        _disp.setCursor(14, 42);
        _disp.print(F("AUTONOMOUS PET"));
        _disp.display();
    }
    _current = _targetShapeFor(EMOTION_IDLE);
    _from    = _current;
    _to      = _current;
    _nextBlink = millis() + random(2500, 5500);
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

EyeShape DisplayEngine::_targetShapeFor(RobotEmotion e) {
    switch (e) {
        case EMOTION_IDLE:             return {36, 40, 10, 36, 40, 10, 6, 0,    false};
        case EMOTION_HELLO:            return {36, 44, 10, 36, 44, 10, 7, -3,   false};
        case EMOTION_LOVE:             return {32, 32, 16, 32, 32, 16, 0, 0,    false};
        case EMOTION_TADA:             return {38, 46, 10, 38, 46, 10, 8, -5,   false};
        case EMOTION_PARTY_DJ:         return {36, 36, 4,  36, 36, 4,  0, 0,    false};
        case EMOTION_CURIOUS_SCAN:     return {30, 42, 8,  30, 42, 8,  5, -2,   false};
        case EMOTION_UHOH_ALERT:       return {28, 28, 6,  28, 28, 6,  4, 2,    false};
        case EMOTION_COOL_SUNGLASSES:  return {48, 34, 6,  48, 34, 6,  0, 0,    false};
        case EMOTION_SLEEP:            return {36, 4,  2,  36, 4,  2,  0, 4,    false};
        case EMOTION_GAMER_PACMAN:     return {28, 28, 14, 28, 28, 14, 5, 0,    false};
        case EMOTION_RAINY_SAD:        return {36, 26, 8,  36, 26, 8,  3, 4,    false};
        case EMOTION_FIRE_RAGE:        return {36, 40, 2,  36, 40, 2,  4, 0,    false};
        case EMOTION_HYPNO_DIZZY:      return {32, 32, 16, 32, 32, 16, 8, 0,    false};
        case EMOTION_KAWAII_CAT:       return {36, 32, 10, 36, 32, 10, 8, 0,    false};
        case EMOTION_JACKPOT_MONEY:    return {36, 36, 4,  36, 36, 4,  0, 0,    false};
        case EMOTION_MATRIX_HACKER:    return {30, 28, 2,  30, 28, 2,  3, 0,    false};
        case EMOTION_KAWAII_KISS:      return {28, 28, 14, 36, 40, 10, 6, 0,    true };
        case EMOTION_FOCUS_STUDY:      return {36, 28, 2,  36, 28, 2,  4, 0,    false};
        case EMOTION_MAGIC_8BALL:      return {28, 36, 8,  28, 36, 8,  5, 0,    false};
        case EMOTION_CLOCK_DISPLAY:    return {24, 16, 4,  24, 16, 4,  3, -8,   false};
        case EMOTION_WEATHER_DISPLAY:  return {24, 16, 4,  24, 16, 4,  3, -8,   false};
        case EMOTION_SENTRY_ALERT:     return {36, 40, 10, 36, 40, 10, 5, 0,    false};
        case EMOTION_SNACK_EAT:        return {36, 24, 8,  36, 24, 8,  6, 0,    false};
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

    // Advance morph
    unsigned long now = millis();
    float t = 1.0f;
    if (_morphDuration > 0) {
        t = (float)(now - _morphStart) / (float)_morphDuration;
        t = constrain(t, 0.0f, 1.0f);
        // Cubic ease in-out
        t = t < 0.5f ? 4.0f*t*t*t : 1.0f - powf(-2.0f*t + 2.0f, 3.0f) / 2.0f;
    }
    _current = _lerpShape(_from, _to, t);

    // Advance blink (suppress during sleep)
    float openRatio = 1.0f;
    if (_currentEmotion != EMOTION_SLEEP) {
        if (_blinking) {
            _blinkProg += 0.22f;
            if (_blinkProg >= 1.0f) {
                _blinking = false;
                _blinkProg = 0.0f;
                _nextBlink = now + random(2800, 6500);
            }
            openRatio = 1.0f - sinf(_blinkProg * M_PI);
            openRatio = constrain(openRatio, 0.0f, 1.0f);
        } else if (now >= _nextBlink) {
            triggerBlink();
        }
    }

    // Scroll message handling
    if (_scrolling) {
        _updateScroll();
        return;
    }

    // Check for active vocal lip sync
    int mouth = audio ? audio->currentMouthShape : -1;
    if (mouth >= 0) {
        renderSpeechFace(mouth, nullptr);
        _disp.display();
        return;
    }

    // Dispatch dedicated artwork for each emotion state
    switch (_currentEmotion) {
        case EMOTION_COOL_SUNGLASSES: renderEmoCoolSunglasses(now / 20); break;
        case EMOTION_LOVE:            renderWalleLoveFace((now / 80) % 6); break;
        case EMOTION_PARTY_DJ:        renderEmoPartyDJ(now / 30); break;
        case EMOTION_CURIOUS_SCAN:    renderCyberHUD(now / 25); break;
        case EMOTION_SLEEP:           renderSleepMode(now / 120); break;
        case EMOTION_GAMER_PACMAN:    renderGamerPacman(now / 40); break;
        case EMOTION_KAWAII_CAT:      renderKawaiiCat(now / 80); break;
        case EMOTION_FOCUS_STUDY:     renderFocusStudy(now / 100); break;
        case EMOTION_RAINY_SAD:       renderRainySad(now / 30); break;
        case EMOTION_FIRE_RAGE:       renderFireRage(now / 40); break;
        case EMOTION_HYPNO_DIZZY:     renderHypnoDizzy(now / 30); break;
        case EMOTION_JACKPOT_MONEY:   renderJackpotMoney(now / 60); break;
        case EMOTION_MATRIX_HACKER:   renderMatrixHacker(now / 40); break;
        case EMOTION_KAWAII_KISS:     renderKawaiiKiss(now / 60); break;
        case EMOTION_MAGIC_8BALL:     renderMagic8Ball(_magic8Answer.c_str()); break;
        case EMOTION_SENTRY_ALERT:    renderSentryAlert(now / 40); break;
        case EMOTION_SNACK_EAT:       renderSnackEat(now / 150); break;
        case EMOTION_RPS_SHOW:        renderRPS(_rpsChoice.c_str()); break;
        case EMOTION_CLOCK_DISPLAY:   renderClockScreen(_timeStr.c_str(), _dateStr.c_str()); break;
        case EMOTION_WEATHER_DISPLAY: renderWeatherScreen(_tempC, _humidity, _weatherCond.c_str()); break;
        case EMOTION_HELLO:
        case EMOTION_TADA:
        case EMOTION_IDLE:
        default:
            renderLivingIdleFace(_gazeX, _gazeY, openRatio);
            break;
    }

    _disp.display();
}

// 1. Living Idle Face (Continuous Sinusoidal Breathing + 2-Layer Parallax)
void DisplayEngine::renderLivingIdleFace(float gazeX, float gazeY, float openRatio) {
    if (!_ready) return;
    _disp.clearDisplay();

    int offX = (int)(gazeX * 12.0f);
    int offY = (int)(gazeY * 8.0f);

    float breath = sinf(millis() * 0.0022f);
    int hBreath  = (int)(breath * 2.0f);
    int curH     = max(2, (int)((EYE_H + hBreath) * openRatio));
    int curW     = EYE_W;

    if (openRatio < 0.15f) {
        _disp.drawFastHLine(EYE_L_CX + offX - curW/2, EYE_CY + offY, curW, SSD1306_WHITE);
        _disp.drawFastHLine(EYE_R_CX + offX - curW/2, EYE_CY + offY, curW, SSD1306_WHITE);
        return;
    }

    int leftX = (EYE_L_CX + offX) - curW / 2;
    int leftY = (EYE_CY + offY) - curH / 2;
    _disp.fillRoundRect(leftX, leftY, curW, curH, min(EYE_R, curH/2), SSD1306_WHITE);

    int rightX = (EYE_R_CX + offX) - curW / 2;
    int rightY = (EYE_CY + offY) - curH / 2;
    _disp.fillRoundRect(rightX, rightY, curW, curH, min(EYE_R, curH/2), SSD1306_WHITE);

    if (curH > 14) {
        int pupilSize = 6;
        int glintSize = 2;
        int pLX = (EYE_L_CX + offX) + (int)(gazeX * 4.0f);
        int pLY = (EYE_CY + offY) + (int)(gazeY * 3.0f);
        _disp.fillCircle(pLX, pLY, pupilSize, SSD1306_BLACK);
        _disp.fillCircle(pLX + 2, pLY - 2, glintSize, SSD1306_WHITE);

        int pRX = (EYE_R_CX + offX) + (int)(gazeX * 4.0f);
        int pRY = (EYE_CY + offY) + (int)(gazeY * 3.0f);
        _disp.fillCircle(pRX, pRY, pupilSize, SSD1306_BLACK);
        _disp.fillCircle(pRX + 2, pRY - 2, glintSize, SSD1306_WHITE);
    }
}

void DisplayEngine::_drawHeart(int cx, int cy, int size) {
    int r = size / 4;
    _disp.fillCircle(cx - r, cy - r, r, SSD1306_WHITE);
    _disp.fillCircle(cx + r, cy - r, r, SSD1306_WHITE);
    _disp.fillTriangle(cx - size/2, cy - r + 1, cx + size/2, cy - r + 1, cx, cy + size/2, SSD1306_WHITE);
}

void DisplayEngine::renderWalleLoveFace(int pulse) {
    if (!_ready) return;
    _disp.clearDisplay();
    _drawHeart(EYE_L_CX, EYE_CY, 24 + pulse);
    _drawHeart(EYE_R_CX, EYE_CY, 24 + pulse);
    _drawHeart(14, 16 + pulse, 8 + pulse);
    _drawHeart(114, 16 + pulse, 8 + pulse);
    _disp.drawCircle(64, 48, 6, SSD1306_WHITE);
    _disp.fillRect(56, 42, 16, 6, SSD1306_BLACK);
}

void DisplayEngine::renderEmoCoolSunglasses(int glintOffset) {
    if (!_ready) return;
    _disp.clearDisplay();
    int frameW = 48;
    int frameH = 34;

    _disp.fillRoundRect(EYE_L_CX - frameW/2, EYE_CY - frameH/2, frameW, frameH, 6, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - frameW/2, EYE_CY - frameH/2, frameW, frameH, 6, SSD1306_WHITE);
    _disp.fillRect(EYE_L_CX + frameW/2 - 4, EYE_CY - 10, 16, 6, SSD1306_WHITE);

    int g1 = (glintOffset % 50) - 10;
    _disp.drawLine(EYE_L_CX - 18 + g1, EYE_CY - 14, EYE_L_CX - 6 + g1, EYE_CY + 14, SSD1306_BLACK);
    _disp.drawLine(EYE_L_CX - 14 + g1, EYE_CY - 14, EYE_L_CX - 2 + g1, EYE_CY + 14, SSD1306_BLACK);
    _disp.drawLine(EYE_R_CX - 18 + g1, EYE_CY - 14, EYE_R_CX - 6 + g1, EYE_CY + 14, SSD1306_BLACK);
    _disp.drawLine(EYE_R_CX - 14 + g1, EYE_CY - 14, EYE_R_CX - 2 + g1, EYE_CY + 14, SSD1306_BLACK);

    _disp.drawLine(56, 52, 72, 48, SSD1306_WHITE);
    _disp.drawLine(72, 48, 76, 44, SSD1306_WHITE);
}

void DisplayEngine::renderEmoPartyDJ(int step) {
    if (!_ready) return;
    _disp.clearDisplay();

    for (int i = 0; i < 4; i++) {
        int barH = 8 + (int)(18 * fabsf(sinf((step * 0.3f) + i * 1.2f)));
        _disp.fillRect(16 + i * 8, 48 - barH, 6, barH, SSD1306_WHITE);
    }
    for (int i = 0; i < 4; i++) {
        int barH = 8 + (int)(18 * fabsf(cosf((step * 0.3f) + i * 1.2f)));
        _disp.fillRect(80 + i * 8, 48 - barH, 6, barH, SSD1306_WHITE);
    }

    _disp.drawCircle(EYE_L_CX, EYE_CY - 4, 14, SSD1306_WHITE);
    _disp.drawCircle(EYE_R_CX, EYE_CY - 4, 14, SSD1306_WHITE);
    _disp.drawCircleHelper(64, 20, 34, 1 | 2, SSD1306_WHITE);
    _disp.drawCircleHelper(64, 20, 35, 1 | 2, SSD1306_WHITE);

    _disp.drawCircle(64, 52, 6, SSD1306_WHITE);
    _disp.fillRect(56, 46, 16, 6, SSD1306_BLACK);
}

void DisplayEngine::renderCyberHUD(int scanY) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.drawRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    _disp.drawRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);

    int sy = scanY % 64;
    _disp.drawFastHLine(0, sy, 128, SSD1306_WHITE);
    _disp.drawFastHLine(0, (sy + 2) % 64, 128, SSD1306_WHITE);

    _disp.drawCircle(EYE_L_CX, EYE_CY, 8, SSD1306_WHITE);
    _disp.drawCircle(EYE_R_CX, EYE_CY, 8, SSD1306_WHITE);
    _disp.drawFastVLine(EYE_L_CX, EYE_CY - 12, 24, SSD1306_WHITE);
    _disp.drawFastHLine(EYE_L_CX - 12, EYE_CY, 24, SSD1306_WHITE);
    _disp.drawFastVLine(EYE_R_CX, EYE_CY - 12, 24, SSD1306_WHITE);
    _disp.drawFastHLine(EYE_R_CX - 12, EYE_CY, 24, SSD1306_WHITE);

    _disp.setTextSize(1);
    _disp.setCursor(18, 54);
    _disp.print(F("SCANNING TARGET..."));
}

void DisplayEngine::renderSleepMode(int zStep) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.drawFastHLine(EYE_L_CX - 16, EYE_CY + 4, 32, SSD1306_WHITE);
    _disp.drawFastHLine(EYE_L_CX - 14, EYE_CY + 5, 28, SSD1306_WHITE);
    _disp.drawFastHLine(EYE_R_CX - 16, EYE_CY + 4, 32, SSD1306_WHITE);
    _disp.drawFastHLine(EYE_R_CX - 14, EYE_CY + 5, 28, SSD1306_WHITE);

    int zOff = (zStep % 28);
    _disp.setTextSize(1);
    _disp.setCursor(96 + (zOff / 4), 30 - zOff);
    _disp.print(F("z"));
    _disp.setTextSize(2);
    _disp.setCursor(106 + (zOff / 3), 22 - zOff);
    _disp.print(F("Z"));

    _disp.drawCircle(64, 52, 4, SSD1306_WHITE);
}

void DisplayEngine::renderGamerPacman(int frame) {
    if (!_ready) return;
    _disp.clearDisplay();
    int px = (frame * 4) % 140 - 20;

    _disp.fillCircle(px, EYE_CY, 18, SSD1306_WHITE);
    int mouthOpen = (frame % 4 < 2) ? 14 : 4;
    _disp.fillTriangle(px, EYE_CY, px + 20, EYE_CY - mouthOpen, px + 20, EYE_CY + mouthOpen, SSD1306_BLACK);

    for (int dot = px + 28; dot < 128; dot += 18) {
        _disp.fillCircle(dot, EYE_CY, 3, SSD1306_WHITE);
    }

    int gx = px - 32;
    int gy = EYE_CY - 16;
    _disp.fillRoundRect(gx, gy, 24, 32, 10, SSD1306_WHITE);
    int footShift = (frame % 2 == 0) ? 0 : 2;
    _disp.fillRect(gx + 2 + footShift, gy + 22, 5, 4, SSD1306_BLACK);
    _disp.fillRect(gx + 11 + footShift, gy + 22, 5, 4, SSD1306_BLACK);
    _disp.fillRect(gx + 19 - footShift, gy + 22, 5, 4, SSD1306_BLACK);
    _disp.fillCircle(gx + 7, gy + 8, 3, SSD1306_BLACK);
    _disp.fillCircle(gx + 17, gy + 8, 3, SSD1306_BLACK);

    _disp.setTextSize(1);
    _disp.setCursor(18, 54);
    _disp.print(F("LEVEL UP! [ 1UP ]"));
}

void DisplayEngine::renderKawaiiCat(int earTwitch) {
    if (!_ready) return;
    _disp.clearDisplay();
    int et = (earTwitch % 6 < 2) ? 2 : 0;
    _disp.fillTriangle(EYE_L_CX - 16, 12, EYE_L_CX - 6, 2 - et, EYE_L_CX + 4, 12, SSD1306_WHITE);
    _disp.fillTriangle(EYE_R_CX - 4, 12, EYE_R_CX + 6, 2 + et, EYE_R_CX + 16, 12, SSD1306_WHITE);

    _disp.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - 14, EYE_W, 32, 10, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - 14, EYE_W, 32, 10, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, EYE_CY, 8, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, EYE_CY, 8, SSD1306_BLACK);
    _disp.fillCircle(EYE_L_CX + 3, EYE_CY - 3, 3, SSD1306_WHITE);
    _disp.fillCircle(EYE_R_CX + 3, EYE_CY - 3, 3, SSD1306_WHITE);

    _disp.drawLine(8, 22, 18, 24, SSD1306_WHITE);
    _disp.drawLine(8, 28, 18, 26, SSD1306_WHITE);
    _disp.drawLine(120, 22, 110, 24, SSD1306_WHITE);
    _disp.drawLine(120, 28, 110, 26, SSD1306_WHITE);

    _disp.drawCircle(60, 48, 3, SSD1306_WHITE);
    _disp.fillRect(57, 45, 6, 3, SSD1306_BLACK);
    _disp.drawCircle(66, 48, 3, SSD1306_WHITE);
    _disp.fillRect(66, 45, 6, 3, SSD1306_BLACK);
}

void DisplayEngine::renderFocusStudy(int tick) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.drawCircle(EYE_L_CX, EYE_CY, 16, SSD1306_WHITE);
    _disp.drawCircle(EYE_R_CX, EYE_CY, 16, SSD1306_WHITE);
    _disp.drawLine(EYE_L_CX + 16, EYE_CY - 4, EYE_R_CX - 16, EYE_CY - 4, SSD1306_WHITE);

    _disp.fillCircle(EYE_L_CX, EYE_CY, 5, SSD1306_WHITE);
    _disp.fillCircle(EYE_R_CX, EYE_CY, 5, SSD1306_WHITE);

    _disp.drawRoundRect(20, 48, 88, 12, 3, SSD1306_WHITE);
    int bar = 4 + (tick % 80);
    _disp.fillRect(22, 50, bar, 8, SSD1306_WHITE);

    _disp.setTextSize(1);
    _disp.setCursor(24, 2);
    _disp.print(F("FOCUS MODE [ 25:00 ]"));
}

void DisplayEngine::renderRainySad(int dropStep) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.fillCircle(50, 8, 6, SSD1306_WHITE);
    _disp.fillCircle(64, 6, 8, SSD1306_WHITE);
    _disp.fillCircle(78, 8, 6, SSD1306_WHITE);
    _disp.fillRoundRect(42, 8, 44, 6, 3, SSD1306_WHITE);

    int dy = 16 + (dropStep % 36);
    _disp.drawLine(48, dy, 48, dy + 3, SSD1306_WHITE);
    _disp.drawLine(64, (dy + 12) % 36 + 16, 64, (dy + 12) % 36 + 19, SSD1306_WHITE);
    _disp.drawLine(80, (dy + 24) % 36 + 16, 80, (dy + 24) % 36 + 27, SSD1306_WHITE);

    _disp.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - 10, EYE_W, 26, 8, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - 10, EYE_W, 26, 8, SSD1306_WHITE);
    _disp.fillTriangle(EYE_L_CX - 18, EYE_CY - 14, EYE_L_CX + 18, EYE_CY - 14, EYE_L_CX - 18, EYE_CY - 2, SSD1306_BLACK);
    _disp.fillTriangle(EYE_R_CX - 18, EYE_CY - 14, EYE_R_CX + 18, EYE_CY - 14, EYE_R_CX + 18, EYE_CY - 2, SSD1306_BLACK);

    _disp.fillCircle(EYE_L_CX - 8, EYE_CY + 18 + (dropStep % 10), 2, SSD1306_WHITE);
    _disp.fillCircle(EYE_R_CX + 8, EYE_CY + 18 + (dropStep % 10), 2, SSD1306_WHITE);

    _disp.drawCircle(64, 56, 4, SSD1306_WHITE);
    _disp.fillRect(58, 56, 12, 6, SSD1306_BLACK);
}

void DisplayEngine::renderFireRage(int flameStep) {
    if (!_ready) return;
    _disp.clearDisplay();

    for (int i = 0; i < 3; i++) {
        int fh1 = 6 + (int)(10 * fabsf(sinf((flameStep * 0.4f) + i)));
        int fh2 = 6 + (int)(10 * fabsf(cosf((flameStep * 0.4f) + i)));
        _disp.fillTriangle(EYE_L_CX - 12 + i*10, EYE_CY - 16, EYE_L_CX - 8 + i*10, EYE_CY - 16 - fh1, EYE_L_CX - 4 + i*10, EYE_CY - 16, SSD1306_WHITE);
        _disp.fillTriangle(EYE_R_CX - 12 + i*10, EYE_CY - 16, EYE_R_CX - 8 + i*10, EYE_CY - 16 - fh2, EYE_R_CX - 4 + i*10, EYE_CY - 16, SSD1306_WHITE);
    }

    _disp.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    _disp.fillTriangle(EYE_L_CX - 20, 2, EYE_L_CX + 20, 2, EYE_L_CX + 20, 20, SSD1306_BLACK);
    _disp.fillTriangle(EYE_R_CX - 20, 2, EYE_R_CX + 20, 2, EYE_R_CX - 20, 20, SSD1306_BLACK);

    _disp.fillRect(EYE_L_CX - 3, EYE_CY - 2, 6, 12, SSD1306_BLACK);
    _disp.fillRect(EYE_R_CX - 3, EYE_CY - 2, 6, 12, SSD1306_BLACK);

    _disp.drawRect(52, 48, 24, 6, SSD1306_WHITE);
    for (int x = 56; x < 76; x += 4) _disp.drawFastVLine(x, 48, 6, SSD1306_WHITE);
}

void DisplayEngine::renderHypnoDizzy(int rot) {
    if (!_ready) return;
    _disp.clearDisplay();

    for (int r = 16; r > 3; r -= 5) {
        int offset = (rot + r * 2) % 8;
        _disp.drawRoundRect(EYE_L_CX - r, EYE_CY - r, r*2, r*2, offset, SSD1306_WHITE);
        _disp.drawRoundRect(EYE_R_CX - r, EYE_CY - r, r*2, r*2, offset, SSD1306_WHITE);
    }

    int sx1 = 64 + (int)(32 * cosf(rot * 0.15f));
    int sy1 = 24 + (int)(16 * sinf(rot * 0.15f));
    _disp.drawPixel(sx1, sy1, SSD1306_WHITE);
    _disp.drawPixel(sx1+1, sy1, SSD1306_WHITE);

    _disp.drawCircle(58, 50, 4, SSD1306_WHITE);
    _disp.fillRect(54, 46, 8, 4, SSD1306_BLACK);
    _disp.drawCircle(66, 50, 4, SSD1306_WHITE);
    _disp.fillRect(66, 50, 8, 4, SSD1306_BLACK);
}

void DisplayEngine::renderJackpotMoney(int coinStep) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.drawRoundRect(EYE_L_CX - 18, EYE_CY - 18, 36, 36, 4, SSD1306_WHITE);
    _disp.drawRoundRect(EYE_R_CX - 18, EYE_CY - 18, 36, 36, 4, SSD1306_WHITE);

    _disp.setTextSize(2);
    _disp.setCursor(EYE_L_CX - 6, EYE_CY - 7);
    _disp.print(F("$"));
    _disp.setCursor(EYE_R_CX - 6, EYE_CY - 7);
    _disp.print(F("$"));

    int cy = 4 + (coinStep % 54);
    _disp.fillCircle(12, cy, 3, SSD1306_WHITE);
    _disp.fillCircle(116, (cy + 18) % 54 + 4, 3, SSD1306_WHITE);

    _disp.setTextSize(1);
    _disp.setCursor(24, 52);
    _disp.print(F("JACKPOT! $$$"));
}

void DisplayEngine::renderMatrixHacker(int frame) {
    if (!_ready) return;
    _disp.clearDisplay();

    for (int col = 6; col < 124; col += 12) {
        int streamY = ((frame * 3) + col * 7) % 50;
        _disp.drawFastVLine(col, streamY, 8, SSD1306_WHITE);
        _disp.drawPixel(col, streamY + 10, SSD1306_WHITE);
    }

    _disp.drawRect(34, 16, 60, 32, SSD1306_WHITE);
    _disp.drawFastHLine(30, 32, 68, SSD1306_WHITE);
    _disp.drawFastVLine(64, 12, 40, SSD1306_WHITE);

    _disp.setTextSize(1);
    _disp.setCursor(22, 54);
    _disp.print(F("[ ACCESS GRANTED ]"));
}

void DisplayEngine::renderKawaiiKiss(int heartFlight) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.drawCircle(EYE_L_CX, EYE_CY + 4, 14, SSD1306_WHITE);
    _disp.fillRect(EYE_L_CX - 16, EYE_CY + 4, 32, 16, SSD1306_BLACK);

    _disp.fillCircle(EYE_R_CX, EYE_CY, 14, SSD1306_WHITE);
    _disp.fillCircle(EYE_R_CX, EYE_CY, 6, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX + 2, EYE_CY - 2, 2, SSD1306_WHITE);

    int hx = 64 + (heartFlight % 50);
    int hy = 44 - (heartFlight % 32);
    _drawHeart(hx, hy, 12);

    _disp.drawCircle(64, 50, 6, SSD1306_WHITE);
    _disp.fillRect(54, 44, 20, 6, SSD1306_BLACK);
}

void DisplayEngine::renderMagic8Ball(const char* answer) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.drawCircle(64, 30, 24, SSD1306_WHITE);
    _disp.fillCircle(64, 30, 14, SSD1306_WHITE);
    _disp.fillTriangle(54, 36, 74, 36, 64, 20, SSD1306_BLACK);

    _disp.setTextSize(1);
    int len = strlen(answer);
    int sx = (128 - (len * 6)) / 2;
    _disp.setCursor(max(4, sx), 56);
    _disp.print(answer);
}

void DisplayEngine::renderSentryAlert(int tick) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - EYE_H/2, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);

    int scanX = 16 + (tick % 96);
    _disp.drawFastVLine(scanX, 0, 64, SSD1306_BLACK);
    _disp.drawFastVLine(scanX + 1, 0, 64, SSD1306_BLACK);

    _disp.drawTriangle(EYE_L_CX - 18, 4, EYE_L_CX + 18, 4, EYE_L_CX + 18, 20, SSD1306_BLACK);
    _disp.drawTriangle(EYE_R_CX - 18, 4, EYE_R_CX + 18, 4, EYE_R_CX - 18, 20, SSD1306_BLACK);

    _disp.setTextSize(1);
    _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(20, 54);
    _disp.print(F("! SENTRY ACTIVE !"));
}

void DisplayEngine::renderSnackEat(int chewFrame) {
    if (!_ready) return;
    _disp.clearDisplay();
    int chew = (chewFrame % 4 < 2) ? 6 : 0;

    _disp.fillRoundRect(EYE_L_CX - EYE_W/2, EYE_CY - 12 + chew/2, EYE_W, 24 - chew, 8, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - EYE_W/2, EYE_CY - 12 + chew/2, EYE_W, 24 - chew, 8, SSD1306_WHITE);

    _disp.fillTriangle(54, 42, 74, 42, 64, 60, SSD1306_WHITE);
    _disp.fillCircle(64, 48, 2, SSD1306_BLACK);
    _disp.fillCircle(60, 45, 1, SSD1306_BLACK);
    _disp.fillCircle(68, 45, 1, SSD1306_BLACK);

    _disp.setTextSize(1);
    _disp.setCursor(34, 4);
    _disp.print(F("YUM YUM! 🍕"));
}

void DisplayEngine::renderRPS(const char* choice) {
    if (!_ready) return;
    _disp.clearDisplay();
    _disp.setTextSize(1);
    _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(18, 4);
    _disp.print(F("PIKU CHOSE:"));

    if (strcmp(choice, "ROCK") == 0) {
        _disp.fillCircle(64, 38, 16, SSD1306_WHITE);
        _disp.drawCircle(64, 38, 18, SSD1306_WHITE);
    } else if (strcmp(choice, "PAPER") == 0) {
        _disp.fillRect(48, 22, 32, 32, SSD1306_WHITE);
        _disp.fillRect(52, 26, 24, 24, SSD1306_BLACK);
    } else {
        _disp.drawLine(46, 22, 82, 54, SSD1306_WHITE);
        _disp.drawLine(46, 23, 82, 55, SSD1306_WHITE);
        _disp.drawLine(46, 54, 82, 22, SSD1306_WHITE);
        _disp.drawLine(46, 55, 82, 23, SSD1306_WHITE);
        _disp.drawCircle(46, 22, 6, SSD1306_WHITE);
        _disp.drawCircle(46, 54, 6, SSD1306_WHITE);
    }

    _disp.setTextSize(1);
    int len = strlen(choice);
    _disp.setCursor((128 - (len * 6)) / 2, 56);
    _disp.print(choice);
}

void DisplayEngine::renderFlappyGame(int bY, float vel, int score, int hi, int pX, int pGY, bool over) {
    if (!_ready) return;
    _disp.clearDisplay();

    _disp.fillRoundRect(18, bY, 14, 10, 3, SSD1306_WHITE);
    _disp.fillCircle(28, bY + 3, 2, SSD1306_BLACK);
    _disp.fillTriangle(32, bY + 5, 36, bY + 7, 32, bY + 9, SSD1306_WHITE);
    int wingY = (vel < 0) ? bY + 6 : bY + 1;
    _disp.drawLine(20, bY + 5, 14, wingY, SSD1306_WHITE);

    _disp.fillRect(pX, 0, 14, pGY, SSD1306_WHITE);
    _disp.fillRect(pX - 2, pGY - 4, 18, 4, SSD1306_WHITE);
    int bottomPipeY = pGY + 28;
    _disp.fillRect(pX, bottomPipeY, 14, 64 - bottomPipeY, SSD1306_WHITE);
    _disp.fillRect(pX - 2, bottomPipeY, 18, 4, SSD1306_WHITE);

    _disp.drawFastHLine(0, 63, 128, SSD1306_WHITE);
    _disp.setTextSize(1);
    _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(4, 2);
    _disp.print(F("SCORE:"));
    _disp.print(score);
    _disp.setCursor(76, 2);
    _disp.print(F("HI:"));
    _disp.print(hi);

    if (over) {
        _disp.fillRoundRect(16, 16, 96, 32, 4, SSD1306_BLACK);
        _disp.drawRoundRect(16, 16, 96, 32, 4, SSD1306_WHITE);
        _disp.setCursor(24, 20);
        _disp.print(F("GAME OVER!"));
        _disp.setCursor(20, 32);
        _disp.print(F("Tap to Retry"));
    }
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

void DisplayEngine::renderSpeechFace(int mouthShape, const char* subtitle) {
    if (!_ready) return;
    renderLivingIdleFace(0, 0, 1.0f);
    _drawMouth(mouthShape);
    if (subtitle) {
        _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
        int len = strlen(subtitle);
        int sx  = (128 - len * 6) / 2;
        _disp.setCursor(max(2, sx), 56);
        _disp.print(subtitle);
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
    // Keep cute mini-eyes alive while scrolling
    float breath = sinf(millis() * 0.003f);
    int eh = 16 + (int)(breath * 2.0f);
    _disp.fillRoundRect(EYE_L_CX - 12, 4, 24, eh, 4, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 12, 4, 24, eh, 4, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_L_CX + 1, 11, 1, SSD1306_WHITE);
    _disp.fillCircle(EYE_R_CX + 1, 11, 1, SSD1306_WHITE);

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
    _disp.fillRoundRect(EYE_L_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
    _disp.fillRoundRect(EYE_R_CX - 12, 4, 24, 18, 4, SSD1306_WHITE);
    _disp.fillCircle(EYE_L_CX, 12, 3, SSD1306_BLACK);
    _disp.fillCircle(EYE_R_CX, 12, 3, SSD1306_BLACK);

    _disp.drawRoundRect(14, 30, 100, 18, 4, SSD1306_WHITE);
    int fillW = muted ? 0 : (vol * 96) / 100;
    if (fillW > 0) _disp.fillRect(16, 32, fillW, 14, SSD1306_WHITE);

    _disp.setTextSize(1); _disp.setTextColor(SSD1306_WHITE);
    _disp.setCursor(30, 52);
    if (vol == 0 || muted) {
        _disp.print(F("MUTED [ X ]"));
    } else {
        _disp.print(F("VOLUME: "));
        _disp.print(vol);
        _disp.print(F("%"));
    }
    _disp.display();
}
