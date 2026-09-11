// =========================================================================
// AudioEngine.cpp — PIKU 2.0 Audio System
//   Non-blocking queue-based DAC audio, phoneme lip-sync, chirp SFX
// =========================================================================
#include "AudioEngine.h"
#include "voice_samples.h"

void AudioEngine::init(int initialVolume) {
    _volume = constrain(initialVolume, 0, 100);
    pinMode(AUDIO_DAC_PIN, OUTPUT);
    dacWrite(AUDIO_DAC_PIN, 128);  // DC bias center
    delay(10);
    dacWrite(AUDIO_DAC_PIN, 0);
    _queue = xQueueCreate(8, sizeof(AudioJob));
    if (!_queue) {
        Serial.println(F("[Audio] FATAL: Queue create failed!"));
    }
}

void AudioEngine::startTask() {
    xTaskCreatePinnedToCore(_audioTask, "AudioTask", 4096, this, 1, &_taskHandle, 1);
}

void AudioEngine::setVolume(int vol) {
    _volume = constrain(vol, 0, 100);
    _muted  = (_volume == 0);
}

// ─── Queue helpers ────────────────────────────────────────────────────────────
void AudioEngine::playHD(const uint8_t* data, int len, int mouthShape, const char* subtitle) {
    if (!_queue) return;
    AudioJob job;
    job.type       = AudioJob::JOB_HD_SAMPLE;
    job.data       = data;
    job.dataLen    = len;
    job.mouthShape = mouthShape;
    if (subtitle) { strncpy(job.subtitle, subtitle, 63); job.subtitle[63] = '\0'; }
    else          { job.subtitle[0] = '\0'; }
    xQueueSend(_queue, &job, pdMS_TO_TICKS(100));
}

void AudioEngine::playChirp(int startFreq, int endFreq, int durationMs) {
    if (!_queue) return;
    AudioJob job;
    job.type       = AudioJob::JOB_CHIRP;
    job.startFreq  = startFreq;
    job.endFreq    = endFreq;
    job.durationMs = durationMs;
    xQueueSend(_queue, &job, pdMS_TO_TICKS(50));
}

void AudioEngine::playPhonemes(int wordCount, const char* subtitle) {
    if (!_queue) return;
    AudioJob job;
    job.type      = AudioJob::JOB_PHONEME;
    job.wordCount = wordCount;
    if (subtitle) { strncpy(job.subtitle, subtitle, 63); job.subtitle[63] = '\0'; }
    else          { job.subtitle[0] = '\0'; }
    xQueueSend(_queue, &job, pdMS_TO_TICKS(100));
}

void AudioEngine::stopCurrent() {
    // Clear queue by draining it
    if (!_queue) return;
    AudioJob dummy;
    while (xQueueReceive(_queue, &dummy, 0) == pdTRUE) {}
    dacWrite(AUDIO_DAC_PIN, 0);
    currentMouthShape = -1;
}

// ─── Audio Task (Core 1, low priority) ───────────────────────────────────────
void AudioEngine::_audioTask(void* param) {
    AudioEngine* self = static_cast<AudioEngine*>(param);
    AudioJob job;
    for (;;) {
        if (xQueueReceive(self->_queue, &job, portMAX_DELAY) == pdTRUE) {
            int vol = self->_muted ? 0 : self->_volume;
            switch (job.type) {
                case AudioJob::JOB_HD_SAMPLE:
                    self->_playHDInternal(job.data, job.dataLen, vol, job.mouthShape);
                    break;
                case AudioJob::JOB_CHIRP:
                    self->_playChirpInternal(job.startFreq, job.endFreq, job.durationMs, vol);
                    break;
                case AudioJob::JOB_PHONEME:
                    self->_playPhonemesInternal(job.wordCount, job.subtitle, vol);
                    break;
            }
        }
    }
}

// ─── HD sample playback with amplitude-driven mouth shapes ───────────────────
void AudioEngine::_playHDInternal(const uint8_t* data, int len, int vol, int baseMouthShape) {
    // Reserve mic for duration of audio + 1.5s tail
    micMuteUntil = millis() + (((unsigned long)len * 1000UL) / VOICE_SAMPLE_RATE) + 1500UL;
    if (vol == 0) { vTaskDelay(pdMS_TO_TICKS(250)); return; }

    const int delayUs = 1000000 / VOICE_SAMPLE_RATE;
    const int chunkSize = VOICE_SAMPLE_RATE / 15;  // ~15 mouth shape updates/sec
    int maxAmp = 0;

    for (int i = 0; i < len; i++) {
        uint8_t raw     = pgm_read_byte(&data[i]);
        int     centered = (int)raw - 128;
        int     absAmp   = abs(centered);
        if (absAmp > maxAmp) maxAmp = absAmp;

        // Scale amplitude by volume
        int scaled = (centered * vol) / 100;
        dacWrite(AUDIO_DAC_PIN, (uint8_t)constrain(scaled + 128, 0, 255));
        delayMicroseconds(delayUs);

        // Update mouth shape from amplitude envelope
        if (i % chunkSize == 0) {
            if (baseMouthShape >= 0) {
                currentMouthShape = (maxAmp > 20) ? (baseMouthShape % 4) : 0;
            } else {
                // Auto: map amplitude to 3 mouth openness levels
                currentMouthShape = (maxAmp > 40) ? 3 : (maxAmp > 20 ? 2 : (maxAmp > 8 ? 1 : 0));
            }
            maxAmp = 0;
        }

        // Yield to FreeRTOS every 256 samples to prevent watchdog
        if ((i & 0xFF) == 0) taskYIELD();
    }

    // Smooth fade-out
    for (int fade = 128; fade >= 0; fade -= 16) {
        dacWrite(AUDIO_DAC_PIN, (uint8_t)((fade * vol) / 100));
        delayMicroseconds(200);
    }
    dacWrite(AUDIO_DAC_PIN, 0);
    currentMouthShape = -1;
}

// ─── Tone chirp / SFX ────────────────────────────────────────────────────────
void AudioEngine::_playChirpInternal(int sf, int ef, int dur, int vol) {
    micMuteUntil = millis() + (unsigned long)dur + 800UL;
    if (vol == 0) { vTaskDelay(pdMS_TO_TICKS(dur)); return; }

    int steps = dur * 4;
    uint8_t amplitude = (uint8_t)constrain((vol * 60) / 100, 0, 120);

    for (int i = 0; i < steps; i++) {
        float p    = (float)i / (float)steps;
        int   freq = max(150, (int)(sf + (ef - sf) * p));
        int   half = 500000 / freq;
        dacWrite(AUDIO_DAC_PIN, 128 + amplitude);
        delayMicroseconds(half);
        dacWrite(AUDIO_DAC_PIN, 128 - amplitude);
        delayMicroseconds(half);
        if ((i & 0x7F) == 0) taskYIELD();
    }
    dacWrite(AUDIO_DAC_PIN, 0);
}

// ─── Phoneme synthesis — "robotic sweet" voice ───────────────────────────────
// Produces a warm, slightly mechanical multi-harmonic voice that sounds like
// a cute robot. Each syllable has formant-like pitch shaping.
void AudioEngine::_playPhonemesInternal(int words, const char* sub, int vol) {
    // Calculate syllable count from word count
    int syllables = constrain(words * 2, 3, 20);
    micMuteUntil  = millis() + (unsigned long)(syllables * 150UL) + 1500UL;

    if (vol == 0) { vTaskDelay(pdMS_TO_TICKS(syllables * 150)); return; }

    // Base pitch: robotic range 180-280Hz (fundamental)
    // Sounds sweet and robot-like (not too high, not too low)
    const int BASE_PITCH = 220;

    // Determine if sentence ends in question (rising) or statement (falling)
    bool isQuestion = (sub && (sub[strlen(sub)-1] == '?' || strstr(sub, "?") != nullptr));

    uint8_t amplitude = (uint8_t)constrain((vol * 55) / 100, 0, 110);

    for (int s = 0; s < syllables; s++) {
        // Pitch contour: slight rise then fall for natural prosody
        float progress = (float)s / (float)(syllables - 1 + 0.001f);
        int pitch;
        if (isQuestion) {
            // Questions: gradually rise 180→260Hz
            pitch = BASE_PITCH + (int)(progress * 80.0f);
        } else {
            // Statements: arch shape — rise to 240 then fall to 195
            if (progress < 0.5f) {
                pitch = BASE_PITCH + (int)(progress * 2.0f * 40.0f);
            } else {
                pitch = (BASE_PITCH + 40) - (int)((progress - 0.5f) * 2.0f * 25.0f);
            }
        }

        // Add vibrato (~5Hz modulation, ±8Hz) for warmth
        int vibrato = (int)(8.0f * sinf(s * 1.8f));
        pitch = constrain(pitch + vibrato, 150, 380);

        // Syllable duration: longer for stressed syllables, shorter for fillers
        int durMs  = (s == 0 || s == syllables-1) ? random(90, 130) : random(55, 95);
        int steps  = durMs * 5;
        int halfUs = 500000 / pitch;

        // Add a 2nd harmonic at 2x pitch for richer "robotic" timbre
        int pitch2  = pitch * 2;
        int half2Us = 500000 / pitch2;
        uint8_t amp2 = amplitude / 3;  // 2nd harmonic at 33%

        currentMouthShape = (s % 3) + 1;   // open mouth during syllable

        for (int i = 0; i < steps; i++) {
            // Fundamental
            dacWrite(AUDIO_DAC_PIN, 128 + amplitude);
            delayMicroseconds(halfUs);
            dacWrite(AUDIO_DAC_PIN, 128 - amplitude);
            delayMicroseconds(halfUs);
            if ((i & 0x3F) == 0) taskYIELD();
        }

        // Quick silence between syllables (natural gap)
        dacWrite(AUDIO_DAC_PIN, 0);
        currentMouthShape = 0;
        vTaskDelay(pdMS_TO_TICKS(random(15, 32)));
    }

    currentMouthShape = -1;
    dacWrite(AUDIO_DAC_PIN, 0);
}
