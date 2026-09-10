#pragma once
#include <Arduino.h>
#include "config.h"
#include "AudioEngine.h"

class SensorEngine {
public:
    using Callback = void(*)();
    void init(AudioEngine* audio);
    void update();
    void setSoundEnabled(bool en) { _soundEnabled = en; }
    bool getSoundEnabled()  const { return _soundEnabled; }
    void onTouchShort(Callback cb)     { _cbTouchShort = cb; }
    void onTouchSustained(Callback cb) { _cbTouchSustained = cb; }
    void onTouchOverpet(Callback cb)   { _cbTouchOverpet = cb; }
    void onDoubleClap(Callback cb)     { _cbDoubleClap = cb; }

private:
    AudioEngine* _audio;
    bool _soundEnabled = false;

    // Touch state
    bool          _petting          = false;
    unsigned long _petStart         = 0;
    unsigned long _lastPetTick      = 0;
    int           _touchDebounce    = 0;
    Callback      _cbTouchShort     = nullptr;
    Callback      _cbTouchSustained = nullptr;
    Callback      _cbTouchOverpet   = nullptr;

    // Clap state
    int           _clapCount        = 0;
    unsigned long _firstClapTime    = 0;
    Callback      _cbDoubleClap     = nullptr;

    void _updateTouch();
    void _updateClap();
};
