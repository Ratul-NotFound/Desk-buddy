#pragma once
#include <Arduino.h>
#include <functional>
#include "config.h"
#include "AudioEngine.h"

class SensorEngine {
public:
    using Callback = std::function<void()>;

    void init(AudioEngine* audio);
    void update();
    void setSoundEnabled(bool en) { _soundEnabled = en; }
    bool getSoundEnabled()  const { return _soundEnabled; }

    // Register callbacks — accepts lambdas with captures
    void onTouchDown(Callback cb)      { _cbTouchDown      = cb; }
    void onTouchShort(Callback cb)     { _cbTouchShort     = cb; }
    void onTouchSustained(Callback cb) { _cbTouchSustained = cb; }
    void onTouchOverpet(Callback cb)   { _cbTouchOverpet   = cb; }
    void onDoubleClap(Callback cb)     { _cbDoubleClap     = cb; }

private:
    AudioEngine* _audio = nullptr;
    bool _soundEnabled  = false;

    // Touch state
    bool          _petting       = false;
    unsigned long _petStart      = 0;
    unsigned long _lastPetTick   = 0;
    int           _touchDebounce = 0;
    Callback      _cbTouchDown;
    Callback      _cbTouchShort;
    Callback      _cbTouchSustained;
    Callback      _cbTouchOverpet;

    // Clap state
    int           _clapCount     = 0;
    unsigned long _firstClapTime = 0;
    bool          _pinReleased   = false;
    Callback      _cbDoubleClap;

    void _updateTouch();
    void _updateClap();
};
