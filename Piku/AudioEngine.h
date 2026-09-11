#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "config.h"

struct AudioJob {
    enum JobType : uint8_t { JOB_HD_SAMPLE, JOB_CHIRP, JOB_PHONEME } type;
    const uint8_t* data     = nullptr;
    int dataLen             = 0;
    int startFreq           = 0;
    int endFreq             = 0;
    int durationMs          = 0;
    int wordCount           = 0;
    char subtitle[64]       = {};
    int mouthShape          = -1;
};

class AudioEngine {
public:
    void init(int initialVolume);
    void startTask();            // launch FreeRTOS task — call once after init()

    void playHD(const uint8_t* data, int len, int mouthShape = -1, const char* subtitle = nullptr);
    void playChirp(int startFreq, int endFreq, int durationMs);
    void playPhonemes(int wordCount, const char* subtitle = nullptr);
    void stopCurrent();          // drain queue and silence DAC

    void setVolume(int vol);
    int  getVolume() const { return _volume; }
    bool isMuted()   const { return _muted; }
    void setMuted(bool m)  { _muted = m; }

    // Shared state read by DisplayEngine for lip-sync
    volatile int           currentMouthShape = -1;
    volatile unsigned long micMuteUntil      = 0;

private:
    QueueHandle_t _queue      = nullptr;
    TaskHandle_t  _taskHandle = nullptr;
    int  _volume = 80;
    bool _muted  = false;

    void _playHDInternal(const uint8_t* data, int len, int vol, int mouthShape);
    void _playChirpInternal(int sf, int ef, int dur, int vol);
    void _playPhonemesInternal(int words, const char* sub, int vol);

    static void _audioTask(void* param);
};
