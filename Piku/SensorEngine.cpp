#include "SensorEngine.h"

void SensorEngine::init(AudioEngine* audio) {
    _audio = audio;
#if ENABLE_SOUND_SENSOR
    pinMode(MIC_DO_PIN, INPUT_PULLUP);
#endif
}

void SensorEngine::update() {
    _updateTouch();
    _updateClap();
}

void SensorEngine::_updateTouch() {
#if ENABLE_TOUCH_PIN
    int val = touchRead(TOUCH_HEAD_PIN);
    unsigned long now = millis();
    if (val < 35 && val > 0) {
        _touchDebounce++;
        if (_touchDebounce >= 2) {
            if (!_petting) {
                _petting   = true;
                _petStart  = now;
                _lastPetTick = now;
                if (_cbTouchDown) _cbTouchDown();
            }
            unsigned long dur = now - _petStart;
            if (dur > 5500) {
                if (now - _lastPetTick > 1800) {
                    _lastPetTick = now;
                    if (_cbTouchOverpet) _cbTouchOverpet();
                }
            } else if (dur > 1200) {
                if (now - _lastPetTick > 2200) {
                    _lastPetTick = now;
                    if (_cbTouchSustained) _cbTouchSustained();
                }
            }
        }
    } else {
        _touchDebounce = 0;
        if (_petting) {
            unsigned long dur = now - _petStart;
            _petting = false;
            if (dur < 1200) {
                if (_cbTouchShort) _cbTouchShort();
            }
        }
    }
#endif
}

void SensorEngine::_updateClap() {
#if ENABLE_SOUND_SENSOR
    if (!_soundEnabled) return;
    unsigned long now = millis();
    if (now < _audio->micMuteUntil) return;

    // Timeout reset if no second clap within window
    if (_clapCount > 0 && (now - _firstClapTime > 750)) {
        _clapCount = 0;
        _pinReleased = false;
    }

    int pinState = digitalRead(MIC_DO_PIN);

    // If waiting for second clap, verify pin went quiet (HIGH) in between
    if (_clapCount == 1) {
        if (pinState == HIGH) {
            _pinReleased = true; // silence detected between claps
        } else if (_pinReleased && (now - _firstClapTime >= 150) && (now - _firstClapTime <= 700)) {
            // Valid second distinct clap!
            _clapCount = 0;
            _pinReleased = false;
            _audio->micMuteUntil = now + 6000;
            if (_cbDoubleClap) _cbDoubleClap();
        }
    } else if (pinState == LOW) {
        // First clap detected
        _clapCount = 1;
        _pinReleased = false;
        _firstClapTime = now;
        _audio->micMuteUntil = now + 120; // ignore clap echo
    }
#endif
}
