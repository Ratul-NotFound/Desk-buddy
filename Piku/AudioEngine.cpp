#include "AudioEngine.h"
#include "voice_samples.h"

void AudioEngine::init(int initialVolume) {
    _volume = constrain(initialVolume, 0, 100);
    pinMode(AUDIO_DAC_PIN, OUTPUT);
    dacWrite(AUDIO_DAC_PIN, 0);
    _queue = xQueueCreate(4, sizeof(AudioJob));
}

void AudioEngine::startTask() {
    xTaskCreatePinnedToCore(_audioTask, "AudioTask", 4096, this, 1, nullptr, 1);
}

void AudioEngine::setVolume(int vol) {
    _volume = constrain(vol, 0, 100);
    _muted  = (_volume == 0);
}

void AudioEngine::playHD(const uint8_t* data, int len, int mouthShape, const char* subtitle) {
    AudioJob job;
    job.type      = AudioJob::JOB_HD_SAMPLE;
    job.data      = data;
    job.dataLen   = len;
    job.mouthShape = mouthShape;
    if (subtitle) {
        strncpy(job.subtitle, subtitle, 63);
        job.subtitle[63] = '\0';
    } else {
        job.subtitle[0] = '\0';
    }
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::playChirp(int startFreq, int endFreq, int durationMs) {
    AudioJob job;
    job.type      = AudioJob::JOB_CHIRP;
    job.startFreq = startFreq;
    job.endFreq   = endFreq;
    job.durationMs = durationMs;
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::playPhonemes(int wordCount, const char* subtitle) {
    AudioJob job;
    job.type      = AudioJob::JOB_PHONEME;
    job.wordCount = wordCount;
    if (subtitle) {
        strncpy(job.subtitle, subtitle, 63);
        job.subtitle[63] = '\0';
    } else {
        job.subtitle[0] = '\0';
    }
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::_audioTask(void* param) {
    AudioEngine* self = static_cast<AudioEngine*>(param);
    AudioJob job;
    for (;;) {
        if (xQueueReceive(self->_queue, &job, portMAX_DELAY) == pdTRUE) {
            int vol = self->_muted ? 0 : self->_volume;
            switch (job.type) {
                case AudioJob::JOB_HD_SAMPLE:
                    self->currentMouthShape = job.mouthShape;
                    self->_playHDInternal(job.data, job.dataLen, vol);
                    self->currentMouthShape = -1;
                    break;
                case AudioJob::JOB_CHIRP:
                    self->_playChirpInternal(job.startFreq, job.endFreq, job.durationMs, vol);
                    break;
                case AudioJob::JOB_PHONEME:
                    self->currentMouthShape = 0;
                    self->_playPhonemesInternal(job.wordCount, job.subtitle, vol);
                    self->currentMouthShape = -1;
                    break;
            }
        }
    }
}

void AudioEngine::_playHDInternal(const uint8_t* data, int len, int vol) {
    micMuteUntil = millis() + (len / VOICE_SAMPLE_RATE * 1000) + 1500;
    if (vol == 0) { delay(300); return; }
    int delayUs = 1000000 / VOICE_SAMPLE_RATE;
    for (int i = 0; i < len; i++) {
        uint8_t raw = pgm_read_byte(&data[i]);
        int centered = (int)raw - 128;
        int scaled = (centered * vol) / 100;
        dacWrite(AUDIO_DAC_PIN, (uint8_t)constrain(scaled + 128, 0, 255));
        delayMicroseconds(delayUs);
    }
    for (int fade = 128; fade >= 0; fade -= 8) {
        dacWrite(AUDIO_DAC_PIN, (uint8_t)((fade * vol) / 100));
        delayMicroseconds(120);
    }
    dacWrite(AUDIO_DAC_PIN, 0);
}

void AudioEngine::_playChirpInternal(int sf, int ef, int dur, int vol) {
    micMuteUntil = millis() + dur + 1000;
    if (vol == 0) { delay(dur); return; }
    int steps = dur * 5;
    for (int i = 0; i < steps; i++) {
        float p    = (float)i / steps;
        int   freq = max(200, sf + (int)((ef - sf) * p));
        int   half = 500000 / freq;
        uint8_t hi = (uint8_t)constrain(128 + (vol * 50) / 100, 0, 255);
        uint8_t lo = (uint8_t)constrain(128 - (vol * 50) / 100, 0, 255);
        dacWrite(AUDIO_DAC_PIN, hi); delayMicroseconds(half);
        dacWrite(AUDIO_DAC_PIN, lo); delayMicroseconds(half);
    }
    dacWrite(AUDIO_DAC_PIN, 0);
}

void AudioEngine::_playPhonemesInternal(int words, const char* sub, int vol) {
    int syllables = constrain(words * 2, 4, 16);
    micMuteUntil = millis() + (syllables * 120) + 1500;
    if (vol == 0) { delay(syllables * 120); return; }
    for (int s = 0; s < syllables; s++) {
        currentMouthShape = s % 4;
        int base = random(600, 1100);
        int tgt  = base + random(-200, 250);
        int dur  = random(50, 90);
        int steps = dur * 4;
        for (int i = 0; i < steps; i++) {
            float p    = (float)i / steps;
            int   freq = max(300, base + (int)((tgt - base) * p));
            int   half = 500000 / freq;
            uint8_t hi = (uint8_t)constrain(128 + (vol * 45) / 100, 0, 255);
            uint8_t lo = (uint8_t)constrain(128 - (vol * 45) / 100, 0, 255);
            dacWrite(AUDIO_DAC_PIN, hi); delayMicroseconds(half);
            dacWrite(AUDIO_DAC_PIN, lo); delayMicroseconds(half);
        }
        dacWrite(AUDIO_DAC_PIN, 0);
        delay(random(15, 35));
    }
    currentMouthShape = -1;
    dacWrite(AUDIO_DAC_PIN, 0);
}
