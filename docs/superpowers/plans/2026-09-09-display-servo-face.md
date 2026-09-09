# Display Face & Servo Head Movement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a responsive, animated vector eye face on the 0.96" SSD1306 OLED display coordinated with smooth, low-current SG90 servo head movements on the ESP32.

**Architecture:** Non-blocking procedural graphics engine paired with a sinusoidal servo motion interpolator. Designed to run safely on single-cable USB power, with modular architecture ready for audio and sound sensor expansion.

**Tech Stack:** C++ / Arduino Core for ESP32, Adafruit SSD1306, Adafruit GFX, ESP32Servo / native LEDC PWM.

**Spec:** `docs/superpowers/specs/2026-09-09-interactive-desk-buddy-design.md`

## Global Constraints
- Target board: ESP32 DevKit.
- Display: 0.96" I2C OLED (SSD1306 128x64, Address 0x3C, SDA=21, SCL=22).
- Servo: SG90 (Signal=GPIO 18, 5V=VIN, GND=GND).
- Power budget: Single USB cable (~500mA limit) — servo speed is strictly eased to avoid brownout.
- Compatibility: Must work out-of-the-box in **Arduino IDE** (single folder with `.ino`) and in **PlatformIO**.

---

### Task 1: Configuration and Hardware Definitions
**Files:**
- Create: `DeskBuddy/config.h`
- Create: `platformio.ini`

**Interfaces:**
- Produces: Pin constants (`OLED_SDA`, `OLED_SCL`, `SERVO_PIN`), display parameters (`SCREEN_WIDTH=128`, `SCREEN_HEIGHT=64`, `OLED_RESET=-1`, `SCREEN_ADDRESS=0x3C`), servo angle bounds (`SERVO_MIN_ANGLE=30`, `SERVO_MAX_ANGLE=150`, `SERVO_CENTER=90`).

- [ ] **Step 1: Write `DeskBuddy/config.h`**
Define hardware pins, timing intervals, and motion limits.

- [ ] **Step 2: Write `platformio.ini`**
Configure ESP32 devkit environment with `Adafruit SSD1306`, `Adafruit GFX Library`, and `ESP32Servo` library dependencies.

---

### Task 2: Expressive Vector Eye Animation Engine
**Files:**
- Create: `DeskBuddy/EyeDisplay.h`
- Create: `DeskBuddy/EyeDisplay.cpp`

**Interfaces:**
- Produces:
  ```cpp
  enum EyeMood { MOOD_IDLE, MOOD_HAPPY, MOOD_SURPRISED, MOOD_SLEEPY, MOOD_CURIOUS };
  class EyeDisplay {
  public:
      bool begin();
      void update();
      void setMood(EyeMood mood);
      void lookAt(int8_t xOffset, int8_t yOffset);
      void blink();
      int8_t getLookX() const;
  };
  ```

- [ ] **Step 1: Define eye geometries and procedural renderers**
Implement rounded rectangle vector eye renderer with pupil offset, eyelid clipping, and arc deformation for happy (`^ ^`) and surprised (`O O`) expressions.

- [ ] **Step 2: Implement autonomous behaviors and saccades**
Add randomized blinking (intervals between 2.5s and 5.0s, duration 150ms) and natural saccadic gaze shifts (left, center, right).

---

### Task 3: Smooth Eased Servo Motion Controller
**Files:**
- Create: `DeskBuddy/ServoMotion.h`
- Create: `DeskBuddy/ServoMotion.cpp`

**Interfaces:**
- Produces:
  ```cpp
  class ServoMotion {
  public:
      void begin(int pin = SERVO_PIN);
      void update();
      void setTargetAngle(float angle);
      void lookWithEyes(int8_t lookX); // Map eye direction (-1..1) to head angle
      void triggerWiggle();            // Happy celebration dance
      float getCurrentAngle() const;
  };
  ```

- [ ] **Step 1: Implement low-current easing interpolation**
Calculate micro-step updates every 20ms using cubic ease-in/out:
$\Delta \theta = (\text{targetAngle} - \text{currentAngle}) \times 0.15f$.

- [ ] **Step 2: Implement coordinated head-and-eye turning**
Map `lookX` gaze direction directly to smooth physical head rotation (e.g. Center = 90°, Left = 65°, Right = 115°).

---

### Task 4: Main Application & Interactive Demo
**Files:**
- Create: `DeskBuddy/DeskBuddy.ino`

**Interfaces:**
- Consumes: `EyeDisplay`, `ServoMotion`, `config.h`.

- [ ] **Step 1: Implement Arduino `setup()` and `loop()`**
Initialize I2C OLED at `0x3C`, attach servo on GPIO 18, and run synchronized state updates.

- [ ] **Step 2: Add Mood Cycle & Interactive Motion Loop**
Cycles naturally through expressive moods (curious glance, happy wiggle, surprised blink, sleepy droop) so the user sees immediate lively motion and facial expressions on their desk!

- [ ] **Step 3: Document Upload Guide**
Provide clear, step-by-step instructions for uploading using either **Arduino IDE** or **PlatformIO**.
