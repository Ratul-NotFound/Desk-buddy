# 📖 PIKU: Complete Setup & User Guide

Welcome to **PIKU** — your living, EMO-style AI desktop companion with Google Gemini 1.5 Flash, Dual-Mode WiFi, and sound/touch reactions!

---

## 🛠️ Step 1: Install Arduino IDE & Required Libraries

1. Open **Arduino IDE**.
2. Go to **Tools ➔ Manage Libraries...** and install:
   - **`Adafruit SSD1306`** (by Adafruit) — Click *Install All* for dependencies.
   - **`Adafruit GFX Library`** (by Adafruit).
   - **`ESP32Servo`** (by Kevin Harrington).

---

## 🚀 Step 2: Configure Flash Partition & Upload

1. Open **`Piku\Piku.ino`** in Arduino IDE.
2. Select your Board: **Tools ➔ Board ➔ esp32 ➔ ESP32 Dev Module**.
3. Select Port: **Tools ➔ Port ➔ (Your ESP32 COM Port)**.
4. **CRITICAL SETTING (Unlocks 3MB Flash):**
   - Go to: **Tools ➔ Partition Scheme**
   - Select: **`Huge APP (3MB No OTA/1MB SPIFFS)`**
5. Click **Upload (➔)**!

---

## 🌐 Step 3: Portable WiFi & Internet Setup Anywhere

PIKU runs in **Simultaneous Dual-Mode WiFi (`WIFI_AP_STA`)**:
- Direct Hotspot AP is **always broadcasted** on **`Piku-WiFi`** (Password: `piku12345`) at **`http://192.168.4.1`**.

### Connecting PIKU to any Local WiFi Router:
1. Connect your smartphone/laptop to WiFi: **`Piku-WiFi`** (Password: `piku12345`).
2. Open your web browser and navigate to: **`http://192.168.4.1`**.
3. Go to the **"📡 WiFi Router"** tab:
   - Click **"🔍 Scan Nearby 2.4GHz Networks"**.
   - Select your WiFi network from the dropdown list.
   - Type your WiFi password and click **"💾 Connect & Save to Flash"**.
4. **PIKU is now online!** It will automatically remember this network on every boot and can be accessed across your local network at:
   - **`http://piku.local`** (via mDNS) or its assigned local IP address!

---

## 🧠 Step 4: Setting Up Google Gemini AI

1. Get a free Google Gemini API Key from Google AI Studio (`https://aistudio.google.com/`).
2. In the PIKU Web Dashboard (`http://piku.local` or `http://192.168.4.1`), go to **"⚙️ Settings"**.
3. Paste your Gemini API Key and click **"💾 Save Gemini Key"**.
4. Go to the **"🧠 AI Brain"** tab:
   - **🎙️ Speech-to-Text:** Tap the microphone icon on your smartphone to speak directly to PIKU!
   - **Emotion & Voice Reactions:** PIKU queries Gemini 1.5 Flash, parses emotions (`[HAPPY]`, `[LOVE]`, `[ANGER]`, `[COOL]`, `[PARTY]`, `[CAT]`), turns its head, speaks its cute 16kHz voice sound, and scrolls the witty answer across the OLED screen!

---

## 🎮 Step 5: How to Interact with PIKU

### 1. 🎤 Sound & Clap Commands (GPIO 19)
- **👏 Single Clap / Sound Spike:** Wakes PIKU up from sleep with alert eyes and spoken greeting: *"Hello! I'm Piku!"*.
- **👏👏 Double Clap (Within 150ms–550ms):** Instantly triggers **DJ Party Mode** with dancing equalizers and celebration wiggles!

### 2. ✋ Capacitive Head Petting (GPIO 4)
- Stroke or touch the wire on GPIO 4: PIKU purrs with Kawaii Cat face, blushing cheeks, and a joyful wiggle dance.

### 3. 🕹️ Playable Mini-Games
- **Flappy Bird:** Press `x` in Serial Monitor or tap *Launch Flappy Bird* in Web UI. Tap head wire or press `x` to flap!
- **Rock Paper Scissors:** Press `r` in Serial Monitor or tap *RPS* in Web UI. PIKU counts down *"1, 2, 3... SHOOT!"* and shows its pick!
- **Magic 8-Ball:** Press `8` in Serial Monitor to get fortune answers.

### 4. 🔊 Master Volume Controls
- **In Web Dashboard:** Slide the **Volume Slider (0% - 100%)** or tap quick presets: **🔇 Mute**, **🔉 40%**, **🔊 80%**, or **📢 100%**.
- **Keyboard Shortcuts (Serial Monitor):**
  - `+` or `]` ➔ Volume Up (+10%) & Ta-Daaa test chirp
  - `-` or `[` ➔ Volume Down (-10%)
  - `0` ➔ Toggle Mute / Unmute
- **On-Screen Volume HUD:** Adjusting volume shows a live animated volume level bar on the OLED display!
- **Persistent Memory:** PIKU automatically saves your preferred volume level to Flash memory so it stays saved across power cycles!

### 5. 🎭 Serial Keyboard Controls (115200 Baud)
- `1` / `h` ➔ Hello Greeting
- `2` / `l` ➔ Love Heart Eyes (`"I Love You!"`)
- `3` / `t` ➔ Ta-Daaa Celebration
- `4` / `p` ➔ DJ Party Beats
- `5` ➔ Curious Cyber HUD Scan
- `6` / `u` ➔ Uh-Oh Alert
- `7` ➔ Sleep Mode
- `g` ➔ Cool Sunglasses
- `b` / `c` ➔ Kawaii Cat with Twitching Ears
- `f` ➔ Fire Rage
- `z` ➔ Hypno Spiral Dizzy
- `m` ➔ Jackpot Money $$$
- `k` ➔ Matrix Hacker Rain
- `v` ➔ Kawaii Kiss with Floating Hearts
- `e` ➔ Feed Pizza Snack
- `a` ➔ Desk Sentry Alarm Mode
- `o` ➔ 25-min Study Timer
- `+` / `]` ➔ Volume Up (+10%)
- `-` / `[` ➔ Volume Down (-10%)
- `0` ➔ Toggle Mute
- `i` ➔ Reset to Normal Biological Idle
