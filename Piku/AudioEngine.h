#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "config.h"

struct AudioJob {
    enum JobType : uint8_t { JOB_HD_SAMPLE, JOB_CHIRP, JOB_PHONEME } type;
    const uint8_t* data;
    int dataLen;
    int startFreq;
    int endFreq;
    int durationMs;
    int wordCount;
    char subtitle[64];
    int mouthShape;
};

class AudioEngine {
public:
    void init(int initialVolume);
    void startTask();            // launch FreeRTOS task — call once after init()
    void playHD(const uint8_t* data, int len, int mouthShape = -1, const char* subtitle = nullptr);
    void playChirp(int startFreq, int endFreq, int durationMs);
    void playPhonemes(int wordCount, const char* subtitle = nullptr);
    void setVolume(int vol);
    int  getVolume() const { return _volume; }
    bool isMuted()   const { return _muted; }
    void setMuted(bool m)  { _muted = m; }

    volatile int           currentMouthShape = -1;
    volatile unsigned long micMuteUntil      = 0;

private:
    QueueHandle_t _queue;
    int  _volume = 80;
    bool _muted  = false;

    void _playHDInternal(const uint8_t* data, int len, int vol, int mouthShape);
    void _playChirpInternal(int sf, int ef, int dur, int vol);
    void _playPhonemesInternal(int words, const char* sub, int vol);

    static void _audioTask(void* param);
};
