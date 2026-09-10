// =========================================================================
// 🤖 PIKU 2.0 — AUTONOMOUS AI DESK COMPANION
//    FreeRTOS Dual-Core | Modular C++ | Gemini AI | Owner Memory
// =========================================================================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <vector>

#include "config.h"
#include "secrets.h"
#include "voice_samples.h"
#include "ServoEngine.h"
#include "AudioEngine.h"
#include "SensorEngine.h"
#include "DisplayEngine.h"
#include "SoulEngine.h"
#include "BrainEngine.h"
#include "NetworkEngine.h"
#include "WebUI.h"

// ─── Module Instances ────────────────────────────────────────────────────────
ServoEngine  servo;
AudioEngine  audio;
SensorEngine sensors;
DisplayEngine disp;
SoulEngine   soul;
BrainEngine  brain;
NetworkEngine net;
WebServer    server(80);

// ─── Persistent Settings ─────────────────────────────────────────────────────
int   gmtOffsetHours = DEFAULT_GMT_OFFSET;
float userLat = 23.8103f, userLon = 90.4125f;
int   masterVolume = 80;
bool  soundEnabled = false;

// ─── Game State ──────────────────────────────────────────────────────────────
int   flappyBirdY  = 28;
float flappyVel    = 0.0f;
int   flappyScore  = 0;
int   flappyPipeX  = 120;
int   flappyPipeGapY = 24;
bool  flappyOver   = false;
unsigned long nextFlappyTick = 0;
int   flappyHiScore = 0;
bool  sentryActive = false;
bool  snackActive  = false;
unsigned long snackEnd = 0;
bool  rpsActive    = false;
unsigned long rpsEnd = 0;
const char* rpsChoice = "ROCK";
const char* magic8Ans  = "YES!";
bool  magic8Active = false;
unsigned long magic8End = 0;

// ─── WiFi Watchdog ───────────────────────────────────────────────────────────
bool  wifiWasConnected = false;
unsigned long nextWifiCheck = 0;
unsigned long nextWeatherCheck = 0;

// ─── FreeRTOS Task: Core 0 — Network / AI ────────────────────────────────────
void networkTask(void*) {
    for (;;) {
        server.handleClient();

        unsigned long now = millis();

        // WiFi watchdog
        if (now >= nextWifiCheck) {
            nextWifiCheck = now + 5000;
            bool connected = net.isStaConnected();
            if (connected && !wifiWasConnected) {
                wifiWasConnected = true;
                Serial.printf("[WiFi] Connected! IP: %s\n", net.getStaIP().c_str());
                net.syncNTP(gmtOffsetHours);
                int t=25,h=65; String cond="Sunny";
                net.fetchWeather(userLat, userLon, &t, &h, &cond);
                brain.currentTempC = t; brain.currentHumidity = h; brain.currentWeather = cond;
                disp.setClockWeather(brain.currentTime, brain.currentDate, t, h, cond);
                disp.startScrollMessage("WiFi OK! Time+Weather synced.", "ONLINE");
                audio.playHD(voice_tada_data, sizeof(voice_tada_data), 3, "WiFi Connected!");
            } else if (!connected) {
                wifiWasConnected = false;
            }
        }

        // Weather resync every 15 minutes when connected
        if (net.isStaConnected() && now >= nextWeatherCheck) {
            nextWeatherCheck = now + 900000;
            int t=brain.currentTempC, h=brain.currentHumidity;
            String cond=brain.currentWeather;
            net.fetchWeather(userLat, userLon, &t, &h, &cond);
            brain.currentTempC = t; brain.currentHumidity = h; brain.currentWeather = cond;
            disp.setClockWeather(brain.currentTime, brain.currentDate, t, h, cond);
        }

        // Update time strings in brain & display
        if (net.timeIsSynced) {
            brain.currentTime = net.getFormattedTime();
            brain.currentDate = net.getFormattedDate();
            disp.setClockWeather(brain.currentTime, brain.currentDate, brain.currentTempC, brain.currentHumidity, brain.currentWeather);
        }

        // Autonomous AI talk (SoulEngine decides when)
        brain.update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── FreeRTOS Task: Core 1 — Companion Soul Loop ────────────────────────────
void soulTask(void*) {
    for (;;) {
        soul.update();
        sensors.update();
        servo.update();

        // Game states
        unsigned long now = millis();
        if (soul.getState() == STATE_GAME_FLAPPY) {
            if (now >= nextFlappyTick) {
                nextFlappyTick = now + 50;
                flappyVel += 1.5f;
                flappyBirdY = constrain((int)(flappyBirdY + flappyVel), 0, 63);
                flappyPipeX -= 3;
                if (flappyPipeX < -16) {
                    flappyPipeX = 128; flappyPipeGapY = random(10, 32);
                    if (!flappyOver) {
                        flappyScore++;
                        if (flappyScore > flappyHiScore) {
                            flappyHiScore = flappyScore;
                            Preferences p; p.begin("piku", false); p.putInt("flappy_hi", flappyHiScore); p.end();
                        }
                    }
                }
                bool hitPipe = (flappyBirdY < flappyPipeGapY || flappyBirdY > flappyPipeGapY + 28)
                               && (24 >= flappyPipeX && 24 <= flappyPipeX + 16);
                if (flappyBirdY <= 0 || flappyBirdY >= 63 || hitPipe) flappyOver = true;
                disp.renderFlappyGame(flappyBirdY, flappyVel, flappyScore, flappyHiScore, flappyPipeX, flappyPipeGapY, flappyOver);
            }
        } else {
            if (snackActive && now >= snackEnd) { snackActive = false; soul.setState(STATE_AWAKE_IDLE); }
            if (rpsActive && now >= rpsEnd)     { rpsActive = false;   soul.setState(STATE_AWAKE_IDLE); }
            if (magic8Active && now >= magic8End){ magic8Active=false; soul.triggerEmotion(EMOTION_IDLE,50,100); }

            disp.update(&audio);
        }

        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

// ─── Web Server Handlers ──────────────────────────────────────────────────────
void handleRoot() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.send(200, "text/html", FPSTR(WEBUI_HTML));
}

void handleCommand() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    soul.feedSnack(); // any interaction boosts affection slightly

    if (server.hasArg("vol")) {
        int v = server.arg("vol").toInt();
        audio.setVolume(v);
        masterVolume = v;
        disp.showVolumeHUD(v, audio.isMuted());
        Preferences p; p.begin("piku",false); p.putInt("volume",v); p.end();
        server.send(200,"text/plain",String(v));
        return;
    }
    if (server.hasArg("steer")) {
        servo.setTarget(constrain(server.arg("steer").toInt(), 40, 140));
        server.send(200,"text/plain","OK"); return;
    }
    if (server.hasArg("mic_en")) {
        soundEnabled = server.arg("mic_en").toInt() == 1;
        sensors.setSoundEnabled(soundEnabled);
        Preferences p; p.begin("piku",false); p.putBool("mic_en",soundEnabled); p.end();
        server.send(200,"text/plain","OK"); return;
    }
    if (server.hasArg("autotalk")) {
        int m = server.arg("autotalk").toInt();
        soul.setAutoTalkInterval(m);
        Preferences p; p.begin("piku",false); p.putInt("auto_talk",m); p.end();
        server.send(200,"text/plain","OK"); return;
    }
    if (server.hasArg("cmd")) {
        String c = server.arg("cmd");
        if      (c=="hello")  { soul.triggerEmotion(EMOTION_HELLO,70,3000); audio.playHD(voice_hello_data,sizeof(voice_hello_data),2,"Hi! I am Piku!"); }
        else if (c=="love")   { soul.triggerEmotion(EMOTION_LOVE,80,4000); audio.playHD(voice_love_data,sizeof(voice_love_data),3,"I Love You!"); }
        else if (c=="party")  { soul.triggerEmotion(EMOTION_PARTY_DJ,100,5000); audio.playHD(voice_party_data,sizeof(voice_party_data),1,"PARTY TIME!"); }
        else if (c=="shades") { soul.triggerEmotion(EMOTION_COOL_SUNGLASSES,70,4500); }
        else if (c=="cat")    { soul.triggerEmotion(EMOTION_KAWAII_CAT,70,4500); audio.playHD(voice_cat_data,sizeof(voice_cat_data),4,"Nya! Meow!"); }
        else if (c=="kiss")   { soul.triggerEmotion(EMOTION_KAWAII_KISS,75,4500); audio.playHD(voice_kiss_data,sizeof(voice_kiss_data),4,"Mwah!"); }
        else if (c=="fire")   { soul.triggerEmotion(EMOTION_FIRE_RAGE,90,4500); audio.playHD(voice_fire_data,sizeof(voice_fire_data),3,"POWER!"); }
        else if (c=="matrix") { soul.triggerEmotion(EMOTION_MATRIX_HACKER,70,4500); audio.playHD(voice_hacker_data,sizeof(voice_hacker_data),2,"ACCESS GRANTED!"); }
        else if (c=="pacman") { soul.triggerEmotion(EMOTION_GAMER_PACMAN,75,4500); audio.playHD(voice_game_data,sizeof(voice_game_data),3,"Level Up!"); }
        else if (c=="money")  { soul.triggerEmotion(EMOTION_JACKPOT_MONEY,80,4500); audio.playHD(voice_money_data,sizeof(voice_money_data),3,"JACKPOT! $$$"); }
        else if (c=="dizzy")  { soul.triggerEmotion(EMOTION_HYPNO_DIZZY,80,4000); audio.playHD(voice_dizzy_data,sizeof(voice_dizzy_data),1,"Head Spinning!"); }
        else if (c=="sad")    { soul.triggerEmotion(EMOTION_RAINY_SAD,60,4000); audio.playHD(voice_sad_data,sizeof(voice_sad_data),0,"Cheer up! <3"); }
        else if (c=="uhoh")   { soul.triggerEmotion(EMOTION_UHOH_ALERT,70,3000); audio.playHD(voice_uhoh_data,sizeof(voice_uhoh_data),1,"Uh-Oh!"); }
        else if (c=="tada")   { soul.triggerEmotion(EMOTION_TADA,85,3500); audio.playHD(voice_tada_data,sizeof(voice_tada_data),3,"Ta-Da!"); }
        else if (c=="clock")  { soul.triggerEmotion(EMOTION_CLOCK_DISPLAY,50,6000); audio.playChirp(800,1400,120); }
        else if (c=="weather"){ soul.triggerEmotion(EMOTION_WEATHER_DISPLAY,50,6000); audio.playChirp(600,1000,150); }
        else if (c=="study")  { soul.triggerEmotion(EMOTION_FOCUS_STUDY,60,15000); audio.playHD(voice_focus_data,sizeof(voice_focus_data),2,"Focus Mode Active!"); }
        else if (c=="sleep")  { soul.setState(STATE_DEEP_SLEEP); soul.triggerEmotion(EMOTION_SLEEP,70,0); audio.playHD(voice_sleep_data,sizeof(voice_sleep_data),0,"Zzz..."); }
        else if (c=="sentry") { soul.setState(STATE_SENTRY_GUARD); sentryActive=true; soul.triggerEmotion(EMOTION_SENTRY_ALERT,90,5000); audio.playHD(voice_sentry_data,sizeof(voice_sentry_data),1,"INTRUDER!"); }
        else if (c=="nod")    { servo.performGesture(GESTURE_NOD); }
        else if (c=="shake")  { servo.performGesture(GESTURE_SHAKE); }
        else if (c=="wiggle") { servo.performGesture(GESTURE_WIGGLE); }
        else if (c=="flap_start") {
            soul.setState(STATE_GAME_FLAPPY); flappyBirdY=28; flappyVel=0; flappyScore=0;
            flappyPipeX=120; flappyPipeGapY=24; flappyOver=false; nextFlappyTick=millis();
            audio.playHD(voice_game_data,sizeof(voice_game_data),3,"Game On!");
        }
        else if (c=="flap_jump") {
            if (soul.getState()==STATE_GAME_FLAPPY) {
                if (flappyOver) { flappyBirdY=28;flappyVel=0;flappyScore=0;flappyPipeX=120;flappyOver=false; }
                else flappyVel = -8.0f;
            }
        }
        else if (c=="rps") {
            const char* choices[]={"ROCK","PAPER","SCISSORS"};
            rpsChoice=choices[random(0,3)];
            disp.setRPSChoice(rpsChoice);
            soul.triggerEmotion(EMOTION_RPS_SHOW,70,4000);
            audio.playHD(voice_rps_data,sizeof(voice_rps_data),3,"1,2,3 SHOOT!");
        }
        else if (c=="8ball") {
            const char* answers[]={"YES!","NO WAY!","MAYBE!","TRY AGAIN","ABSOLUTELY"};
            magic8Ans=answers[random(0,5)];
            disp.setMagic8Answer(magic8Ans);
            soul.triggerEmotion(EMOTION_MAGIC_8BALL,60,4500);
            audio.playChirp(500,1500,200);
        }
        else if (c=="snack") {
            soul.feedSnack();
            soul.triggerEmotion(EMOTION_SNACK_EAT,70,3500);
            audio.playHD(voice_snack_data,sizeof(voice_snack_data),3,"YUM YUM!");
        }
    }
    server.send(200,"text/plain","OK");
}

void handleStatus() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String j = "{";
    j += "\"sta_connected\":"   + String(net.isStaConnected()?"true":"false") + ",";
    j += "\"sta_ip\":\""        + net.getStaIP() + "\",";
    j += "\"ap_ip\":\""         + net.getApIP() + "\",";
    j += "\"time_str\":\""      + brain.currentTime + "\",";
    j += "\"date_str\":\""      + brain.currentDate + "\",";
    j += "\"affection\":"        + String(soul.getAffection()) + ",";
    j += "\"energy\":"           + String(soul.getEnergy()) + ",";
    j += "\"hunger\":"           + String(soul.getHunger()) + ",";
    j += "\"servo_angle\":"      + String((int)servo.getCurrent()) + ",";
    j += "\"temp_c\":"           + String(brain.currentTempC) + ",";
    j += "\"humidity\":"         + String(brain.currentHumidity) + ",";
    j += "\"weather\":\""        + brain.currentWeather + "\",";
    j += "\"volume\":"           + String(audio.getVolume()) + ",";
    j += "\"is_muted\":"         + String(audio.isMuted()?"true":"false") + ",";
    j += "\"mic_enabled\":"      + String(sensors.getSoundEnabled()?"true":"false") + ",";
    j += "\"flappy_hi\":"        + String(flappyHiScore) + ",";
    j += "\"key_count\":"        + String(brain.getKeyCount()) + ",";
    j += "\"active_key\":"       + String(brain.getActiveKeyIndex()+1) + ",";
    j += "\"owner_name\":\""     + brain.getOwnerName() + "\",";
    j += "\"onboarding_done\":"  + String(brain.isOnboardingDone()?"true":"false") + ",";
    j += "\"auto_talk_min\":"    + String(soul.getAutoTalkIntervalMinutes()) + ",";
    // Clean last reply of emotion tags for display
    String lr = brain.getLastReply();
    int cb = lr.indexOf(']');
    if (cb != -1) { lr = lr.substring(cb+1); lr.trim(); }
    int rm = lr.indexOf("[REMEMBER");
    if (rm != -1) lr = lr.substring(0, rm);
    lr.trim();
    j += "\"last_reply\":\""     + lr + "\"";
    j += "}";
    server.send(200,"application/json",j);
}

void handleWifiScan() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.send(200,"application/json",net.scanNetworks());
}

void handleWifiSave() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String body = server.arg("plain");
    int si = body.indexOf("\"ssid\":\"");
    int pi = body.indexOf("\"pass\":\"");
    if (si == -1) { server.send(400,"text/plain","Bad request"); return; }
    String ssid = body.substring(si+8, body.indexOf('"', si+8));
    String pass = (pi!=-1) ? body.substring(pi+8, body.indexOf('"', pi+8)) : "";
    net.saveStaCreds(ssid, pass);
    net.connectStation(ssid, pass, gmtOffsetHours);
    server.send(200,"text/plain","OK");
}

void handleGeminiKey() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String raw = server.arg("plain");
    raw.replace("\n",","); raw.replace("\r","");
    brain.saveKeyPool(raw);
    server.send(200,"text/plain","OK");
}

void handleGeminiAsk() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String prompt = server.arg("plain");
    if (prompt.length() == 0) prompt = server.arg("q");
    if (prompt.length() == 0) { server.send(400,"text/plain","Empty prompt"); return; }
    String reply = brain.askGemini(prompt);
    // Clean for HTTP response
    int cb = reply.indexOf(']');
    if (cb != -1) { reply = reply.substring(cb+1); reply.trim(); }
    int rm = reply.indexOf("[REMEMBER");
    if (rm != -1) reply = reply.substring(0, rm);
    reply.trim();
    server.send(200,"text/plain",reply);
}

void handleSettingsSave() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String body = server.arg("plain");
    // Parse tz, lat, lon from JSON
    int ti = body.indexOf("\"tz\":");
    int lai = body.indexOf("\"lat\":");
    int loi = body.indexOf("\"lon\":");
    if (ti != -1) {
        int end = body.indexOf(',', ti);
        if (end == -1) end = body.indexOf('}', ti);
        if (end != -1) gmtOffsetHours = (int)body.substring(ti+5, end).toFloat();
    }
    if (lai != -1) {
        int end = body.indexOf(',', lai);
        if (end == -1) end = body.indexOf('}', lai);
        if (end != -1) userLat = body.substring(lai+6, end).toFloat();
    }
    if (loi != -1) {
        int end = body.indexOf(',', loi);
        if (end == -1) end = body.indexOf('}', loi);
        if (end != -1) userLon = body.substring(loi+6, end).toFloat();
    }
    Preferences p; p.begin("piku",false);
    p.putInt("gmt_offset",(int)gmtOffsetHours);
    p.putFloat("user_lat",userLat);
    p.putFloat("user_lon",userLon);
    p.end();
    server.send(200,"text/plain","OK");
}

void handleSync() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    net.syncNTP(gmtOffsetHours);
    int t=brain.currentTempC, h=brain.currentHumidity;
    String cond=brain.currentWeather;
    net.fetchWeather(userLat,userLon,&t,&h,&cond);
    brain.currentTempC=t; brain.currentHumidity=h; brain.currentWeather=cond;
    disp.setClockWeather(brain.currentTime, brain.currentDate, t, h, cond);
    server.send(200,"text/plain","OK");
}

void handleBillboard() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String msg = server.arg("plain");
    if (msg.length() > 0) disp.startScrollMessage(msg, "MSG");
    server.send(200,"text/plain","OK");
}

void handleOwnerName() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    String name = server.arg("plain");
    name.trim();
    if (name.length() > 0 && name.length() < 32) {
        brain.setOwnerName(name);
        brain.completeOnboarding();
        audio.playHD(voice_hello_data, sizeof(voice_hello_data), 2, ("Hi, "+name+"!").c_str());
        soul.triggerEmotion(EMOTION_HELLO, 80, 3000);
        server.send(200,"text/plain","OK");
    } else {
        server.send(400,"text/plain","Invalid name");
    }
}

void handleOwnerReset() {
    server.sendHeader("Access-Control-Allow-Origin","*");
    Preferences p; p.begin("piku",false);
    p.remove("owner_name"); p.remove("owner_facts"); p.remove("onboarding");
    p.end();
    server.send(200,"text/plain","OK");
}

// ─── setup() — Hardware init + FreeRTOS launch ───────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(400);
    Serial.println(F("\n=== PIKU 2.0 BOOTING ==="));

    // 1. Load NVS settings
    Preferences prefs; prefs.begin("piku", true);
    gmtOffsetHours  = prefs.getInt("gmt_offset", DEFAULT_GMT_OFFSET);
    userLat         = prefs.getFloat("user_lat", 23.8103f);
    userLon         = prefs.getFloat("user_lon", 90.4125f);
    masterVolume    = prefs.getInt("volume", 80);
    soundEnabled    = prefs.getBool("mic_en", false);
    flappyHiScore   = prefs.getInt("flappy_hi", 0);
    String storedSSID = prefs.getString("sta_ssid", "");
    String storedPass = prefs.getString("sta_pass", "");
    int    autoTalkMin = prefs.getInt("auto_talk", 10);
    prefs.end();

    // 2. Init modules
    servo.init();
    audio.init(masterVolume);
    disp.init();
    sensors.init(&audio);
    soul.init(&disp, &audio, &servo);
    brain.init(&soul, &audio);
    soul.setAutoTalkInterval(autoTalkMin);
    sensors.setSoundEnabled(soundEnabled);

    // 3. Wire sensor callbacks to SoulEngine
    sensors.onTouchDown([]() {
        if (soul.getState() == STATE_GAME_FLAPPY) {
            if (flappyOver) { flappyBirdY = 28; flappyVel = 0; flappyScore = 0; flappyPipeX = 120; flappyOver = false; }
            else flappyVel = -8.0f;
        }
    });
    sensors.onTouchShort([]() {
        if (soul.getState() != STATE_GAME_FLAPPY) {
            soul.onTouchShort();
        }
    });
    sensors.onTouchSustained([]() { soul.onTouchSustained(); });
    sensors.onTouchOverpet([]()   { soul.onTouchOverpet(); });
    sensors.onDoubleClap([]()     { soul.onDoubleClap(); });

    // 4. Start WiFi
    net.init(AP_DEFAULT_SSID, AP_DEFAULT_PASS, MDNS_HOSTNAME);
    if (storedSSID.length() > 0) {
        net.connectStation(storedSSID, storedPass, gmtOffsetHours);
    }

    // 5. Web server routes
    server.on("/",                  HTTP_GET,  handleRoot);
    server.on("/api",               HTTP_GET,  handleCommand);
    server.on("/api/status",        HTTP_GET,  handleStatus);
    server.on("/api/gemini/ask",    HTTP_POST, handleGeminiAsk);
    server.on("/api/gemini/ask",    HTTP_GET,  handleGeminiAsk);
    server.on("/api/gemini/key",    HTTP_POST, handleGeminiKey);
    server.on("/api/wifi/scan",     HTTP_POST, handleWifiScan);
    server.on("/api/wifi/save",     HTTP_POST, handleWifiSave);
    server.on("/api/settings/save", HTTP_POST, handleSettingsSave);
    server.on("/api/billboard",     HTTP_POST, handleBillboard);
    server.on("/api/sync",          HTTP_POST, handleSync);
    server.on("/api/owner/name",    HTTP_POST, handleOwnerName);
    server.on("/api/owner/reset",   HTTP_POST, handleOwnerReset);
    server.begin();
    Serial.println(F("[HTTP] Server started on port 80"));

    // 6. Audio task on Core 1 (low priority), then start other tasks
    audio.startTask();

    // 7. Launch Core 0 and Core 1 FreeRTOS tasks
    xTaskCreatePinnedToCore(networkTask, "NetTask",  8192, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(soulTask,    "SoulTask", 6144, nullptr, 2, nullptr, 1);

    // 8. Startup greeting & onboarding
    delay(500);
    if (!brain.isOnboardingDone()) {
        audio.playHD(voice_hello_data, sizeof(voice_hello_data), 2, "Hi! What's your name?");
        soul.triggerEmotion(EMOTION_HELLO, 80, 4000);
        disp.startScrollMessage("Hi! What's your name?", "NEW FRIEND");
    } else {
        String greet = "Hi, " + brain.getOwnerName() + "!";
        audio.playHD(voice_hello_data, sizeof(voice_hello_data), 2, greet.c_str());
        soul.triggerEmotion(EMOTION_HELLO, 80, 3000);
    }

    Serial.println(F("=== PIKU 2.0 READY ==="));
}

void loop() {
    // Empty — all work done in FreeRTOS tasks
    vTaskDelay(portMAX_DELAY);
}
