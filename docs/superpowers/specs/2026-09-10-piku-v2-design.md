# PIKU 2.0 — Autonomous AI Desk Companion Design Spec

## Overview

Rebuild PIKU from a command-driven display toy into a fully autonomous, emotionally intelligent desk companion that behaves like EMO. The hardware is fixed: ESP32 DevKit V1, 0.96" SSD1306 OLED (128×64), SG90 servo, PAM8403 amp + 3W speaker (GPIO 25 DAC), KY-038 mic (GPIO 19), capacitive touch (GPIO 4). No new hardware is added.

The current `Piku.ino` is a 2,804-line monolith with blocking audio. This redesign restructures it into modular C++ files under the `Piku/` Arduino sketch directory and introduces FreeRTOS dual-core task separation, an Autonomous Soul Engine, persistent owner memory, smooth OLED animation, and a richer Gemini AI integration.

---

## Core Design Goals

1. **Autonomous Life** — Piku speaks, moves, and reacts without the user doing anything
2. **Persistent Memory** — Piku remembers the owner's name and conversation history across reboots
3. **Smooth Expressions** — OLED eye shapes interpolate smoothly (no instant jumps)
4. **Non-blocking Audio** — Audio plays via FreeRTOS queue; web server + servo never freeze
5. **Better AI Integration** — Deeper Gemini context, emotion intensity, spontaneous AI calls
6. **Clean Simple Web UI** — Lightweight, minimal, functional — no heavy CSS

---

## Architecture

### FreeRTOS Dual-Core Split

```
Core 0 (Protocol / Network)          Core 1 (Companion / Soul)
─────────────────────────────         ──────────────────────────────
WebServer (HTTP)                      OLED Render Loop (30 FPS)
WiFi watchdog                         Servo Motion Engine
Gemini HTTPS client                   DAC Audio Queue (non-blocking)
NTP + weather sync                    Touch sensor polling
Soul Engine timer (triggers AI calls) Clap sensor polling
NVS memory reads/writes               Blink + Saccade engine
                                      Behaviour Scheduler
```

Core 1 runs a tight 33ms render loop. Core 0 handles all blocking network I/O in its own task.

### File Structure (Arduino Sketch)

```
Piku/
├── Piku.ino              <- setup(): init hardware, FreeRTOS task launch; loop(): empty
├── config.h              <- Pin defs, servo limits (unchanged)
├── secrets.h             <- Gemini API keys (unchanged, gitignored)
├── voice_samples.h       <- 21 PROGMEM audio arrays (unchanged)
|
├── SoulEngine.h/cpp      <- Autonomous behaviour scheduler, emotional state machine
├── BrainEngine.h/cpp     <- Gemini client, prompt builder, NVS owner memory
├── DisplayEngine.h/cpp   <- OLED keyframe animator, smooth morphing, all render functions
├── AudioEngine.h/cpp     <- FreeRTOS DAC queue, HD sample player, phoneme synth, chirp
├── ServoEngine.h/cpp     <- Smooth servo motion with cubic easing, wiggle choreography
├── SensorEngine.h/cpp    <- Capacitive touch multi-stage logic, noise-isolated clap detector
├── NetworkEngine.h/cpp   <- WiFi AP+STA, mDNS, NTP, Open-Meteo weather
└── WebUI.h               <- Embedded minimal HTML/CSS/JS as PROGMEM const char[]
```

---

## Modules

### 1. SoulEngine — Autonomous Behaviour Scheduler

**Emotional State:** A struct `EmotionalState` holds:
- `currentEmotion` (enum, 24 types)
- `emotionIntensity` (0-100, drives servo angle and wiggle threshold)
- `moodBaseline` (HAPPY / NEUTRAL / SAD — shifts over hours)
- `lastInteractionMs`
- `robotState` (enum CompanionState)

**Built-in spontaneous behaviours:**

| Trigger | Cooldown | Action |
|---|---|---|
| Idle > 5 min | 10 min | Piku says something curious via AI |
| Idle > 15 min | 30 min | Yawn + sleep nudge |
| Time = morning (6-9 AM) | 1 day | Morning greeting via AI |
| Time = evening (18-20) | 1 day | Evening check-in via AI |
| Weather change detected | 1 hour | React to weather |
| Hunger < 20% | 15 min | Sad hunger reminder |
| Energy < 10% | — | Fall asleep |
| Double clap | 5 sec | DJ Party Mode |
| Random 8% chance per 3 min | 20 min | Random personality moment |

**Spontaneous Talk Timer**: Configurable interval (default 10 min) triggers an unprompted Gemini call. Topics drawn from: "Comment on the time/weather", "Ask how owner is doing", "Share a fun fact", "Say something funny", "Ask what they're working on". Interval stored in NVS, adjustable from web UI (Off / 5 / 10 / 20 / 30 min).

### 2. BrainEngine — Gemini AI + Owner Memory

**Owner Profile (NVS-persisted):**
```cpp
struct OwnerProfile {
    char name[32];
    bool onboardingDone;
    char knownFacts[512];   // JSON blob of extracted facts
    int conversationTurns;
};
```

**First-Boot Onboarding:** If `onboardingDone == false`, web UI shows Piku asking owner's name. Response stored in NVS. Piku plays `voice_hello_data` and speaks the name back.

**Short-Term Context:** Ring buffer of last 6 user/piku turn pairs in RAM.

**Gemini Prompt Structure:**
```
You are PIKU, a living AI desk companion robot. Personality: playful, witty, affectionate, curious.
Owner: [name]. Known facts: [knownFacts].
State: Hunger=[n]%, Energy=[n]%, Affection=[n]%. Time: [HH:MM]. Weather: [temp]C [condition].
Rules:
1. Start with EXACTLY ONE emotion tag: [HAPPY:n], [LOVE:n], [CURIOUS:n], [PARTY:n], [CAT:n],
   [COOL:n], [HACKER:n], [KISS:n], [ANGER:n], [SLEEPY:n], [SAD:n] — n = intensity 0-100.
2. Max 18 words. Natural, alive, punchy.
3. If you learn something important about the owner: append [REMEMBER: fact]
Recent dialogue: [last 6 turns]
User says: [prompt]
```

**Emotion Intensity Thresholds:**
- >= 80 → wiggle + strong servo gesture
- 50-79 → head tilt
- < 50 → subtle response only

**`[REMEMBER: ...]` parsing:** BrainEngine extracts and appends to `knownFacts` NVS string.

### 3. DisplayEngine — Smooth Keyframe OLED Animator

**Parametric Eye Renderer:**
All expressions described as `EyeShape` parameter sets. `morphToShape(EyeShape target, int durationMs)` lerps current params to target over time:
```cpp
struct EyeShape {
    float widthL, heightL, radiusL;
    float widthR, heightR, radiusR;
    float pupilSize;
    float offsetY;
    bool  winkLeft;
};
```

**Overlay Layer:** Separate pass renders: blush circles (love/cat/kiss), stars (party), Z letters (sleep), rain drops (sad), sweat drops (dizzy). Overlays stack on any base expression without requiring a full expression change.

**Blink:** Independent timer. Lerps eye height to ~0 and back over 120ms. Asymmetric timing (left blinks 20ms before right for realism).

**Mouth Animation:** `MouthState` enum: NEUTRAL, SMALL_O, WIDE_O, SMILE, FLAT, TALK_1, TALK_2, TALK_3. During HD audio: mouth shape driven by audio sample amplitude. During phoneme speech: cycles TALK_1→2→3→2→1 at 120ms/frame.

**Frame Rate:** Core 1 renders at 30 FPS (33ms budget). `display.display()` only called from Core 1.

### 4. AudioEngine — Non-blocking DAC Queue

**FreeRTOS Queue:** `audioQueue` holds `AudioJob` structs:
```cpp
struct AudioJob {
    enum JobType { JOB_HD_SAMPLE, JOB_CHIRP, JOB_PHONEME } type;
    const uint8_t* data;
    int dataLen;
    int startFreq, endFreq, durationMs;  // chirp params
    int wordCount;                         // phoneme params
    char subtitle[64];
    int mouthShape;
};
```

Audio task on Core 1 dequeues jobs and plays them, setting `micMuteUntil` for the duration. Mouth shape is written to a `volatile int` that DisplayEngine reads each frame.

### 5. ServoEngine

Cubic easing unchanged. New named gesture lookup: `HEAD_TILT_LEFT, HEAD_TILT_RIGHT, WIGGLE, NOD, SHAKE, CURIOUS_TILT`. Called by SoulEngine/BrainEngine based on emotion + intensity.

### 6. SensorEngine

Multi-stage touch and noise-isolated clap detection unchanged in logic. Events now fire through `SoulEngine::onTouchEvent(duration)` and `SoulEngine::onDoubleClap()` instead of setting globals directly.

### 7. NetworkEngine

Extracted from monolith: `connectStation()`, `syncNTP()`, `fetchWeather()`. Weather changes delivered to SoulEngine for mood-reactive behaviours.

### 8. Web UI — Lightweight Minimal Interface

**Design:** Clean dark background (#111), white text, one accent color (#00d4aa teal). No backdrop-filter. No CDN dependencies. System fonts. Total page size target < 12 KB.

**4 Tabs:**

**Tab 0 — Status:** Connection pill, live clock/weather, vitals bars (Affection/Energy/Hunger), last AI response, spontaneous talk interval selector.

**Tab 1 — Chat:** Chat log, text input + mic + Ask button, browser TTS toggle, quick-chip buttons.

**Tab 2 — Controls:** Volume slider + presets, expression grid (4×3), mini-game buttons, servo slider, OLED billboard input.

**Tab 3 — Settings:** Owner name + Reset, Gemini key pool, sound sensor toggle, timezone + location, WiFi connect.

**Telemetry:** 2-second polling of `/api/status`.

---

## NVS Key Map (namespace: "piku")

| Key | Type | Contents |
|---|---|---|
| `sta_ssid` | String | WiFi SSID |
| `sta_pass` | String | WiFi password |
| `gmt_offset` | Int | Timezone offset hours |
| `user_lat` | Float | Latitude |
| `user_lon` | Float | Longitude |
| `volume` | Int | Master volume 0-100 |
| `mic_en` | Bool | Sound sensor enabled |
| `flappy_hi` | Int | Flappy Bird high score |
| `gem_keys` | String | Comma-separated Gemini keys |
| `owner_name` | String | Owner name |
| `owner_facts` | String | Known facts JSON blob |
| `onboarding` | Bool | First boot complete |
| `auto_talk` | Int | Spontaneous talk interval minutes (0=off) |

---

## Global Constraints

- Platform: ESP32 DevKit V1, Arduino framework
- Libraries: Adafruit SSD1306 ^2.5.7, Adafruit GFX ^1.11.5, ESP32Servo ^1.1.0
- Partition: "Huge APP (3MB No OTA/1MB SPIFFS)" — add `board_build.partitions = huge_app.csv` to platformio.ini
- `voice_samples.h` arrays: PROGMEM only — never loaded to RAM
- Audio rate: 16,000 samples/sec
- OLED: 0x3C, I2C 400kHz fast mode
- Servo limits: 40-140 degrees, center 90, easing 0.15f
- Gemini max response: 18 words
- Web UI: Vanilla HTML/CSS/JS only — no external libraries or CDN
- Language: English only
- Serial baud: 115200
