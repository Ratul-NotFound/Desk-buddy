# SDD ledger — plan: docs/superpowers/plans/2026-09-10-piku-v2-implementation.md

MERGE_BASE = 8e73be04f09c1aba4c5c138f0d43719649aa2cba
Plan commit = 9c97c9d4c1f37d64c1e20810f06c593634c79c3b

## Pre-flight Interface Scan

| Tasks | Shared file/interface | Finding |
|---|---|---|
| T1→T2 | AudioEngine imports config.h | T1 must ensure config.h has AUDIO_DAC_PIN, VOICE_SAMPLE_RATE |
| T2→T3 | SensorEngine.init(AudioEngine*) + micMuteUntil | T2 produces `volatile unsigned long micMuteUntil` on AudioEngine; T3 consumes it. Consistent. |
| T2→T4 | DisplayEngine.update(AudioEngine*) uses currentMouthShape | T2 produces `volatile int currentMouthShape` on AudioEngine; T4 reads it. Consistent. |
| T4→T5 | SoulEngine calls disp->morphToEmotion(RobotEmotion, int) | T4 produces `morphToEmotion(RobotEmotion e, int durationMs)`. T5 calls same. Consistent. |
| T5→T6 | BrainEngine calls soul->hasPendingAIRequest(), consumeAIRequest(), setAIThinking(), onAIResponseReceived() | T5 produces all. T6 consumes. Consistent. |
| T5→T9 | SoulEngine exposes getAutoTalkIntervalMinutes(), setAutoTalkInterval(int) | T5 produces. T9 uses in web handler. Consistent. |
| T6→T9 | BrainEngine exposes askGemini(), getOwnerName(), isOnboardingDone(), getLastReply(), getKeyCount() | T6 produces. T9 uses. Consistent. |
| T7→T9 | NetworkEngine exposes isStaConnected(), getStaIP(), getApIP(), getFormattedTime(), getFormattedDate() | T7 produces. T9 uses. Consistent. |
| T8→T9 | WebUI.h exports WEBUI_HTML PROGMEM const char[] | T8 produces. T9 uses via FPSTR(). Consistent. |
| T1→T9 | ServoEngine::getCurrent() used in handleStatus() | T1 produces. T9 uses. Consistent. |

No conflicts detected between tasks or Global Constraints. Proceeding.

## Rulings
- Ruling: config.h must define AUDIO_DAC_PIN, VOICE_SAMPLE_RATE, SERVO_EASING, AP_DEFAULT_SSID, AP_DEFAULT_PASS, MDNS_HOSTNAME, DEFAULT_GMT_OFFSET, NTP_SERVER, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, OLED_RESET, OLED_SDA, OLED_SCL, EYE_L_CX, EYE_R_CX, EYE_CY, EYE_W, EYE_H, EYE_R, MIC_DO_PIN, TOUCH_HEAD_PIN, SERVO_CENTER, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, ENABLE_SOUND_SENSOR, ENABLE_TOUCH_PIN. Task 1 implementer must verify config.h and add any missing defines. — Why: modules reference these without re-defining them. — Cost if wrong: compile errors in dependent tasks.

## Progress
- Task 1: complete — commit cf466c8 (scaffold + ServoEngine module with FreeRTOS-safe motion)
- Task 2: complete — commit c03c1d3 (AudioEngine with FreeRTOS non-blocking DAC queue)
- Task 3: complete — commit b507391 (SensorEngine with callback-based touch and clap events)
- Task 4: complete — commit 35606e8 (DisplayEngine with parametric smooth OLED keyframe animation)
- Task 5: complete — commit 1a8ca4d (SoulEngine autonomous behaviour scheduler + emotional state machine)
- Task 6: complete — commit 15ba24a (BrainEngine with Gemini AI, owner memory, context, [REMEMBER] extraction)
- Task 7: complete — commit ca8550b (NetworkEngine WiFi AP+STA, NTP, Open-Meteo weather, mDNS)
- Task 8: complete — commit 3cdc15b (WebUI lightweight minimal dashboard <12KB, 4 tabs, clean dark theme)
- Task 9: complete — commit 42e49cc (main FreeRTOS dual-core integration — all modules wired, web server, game logic)
