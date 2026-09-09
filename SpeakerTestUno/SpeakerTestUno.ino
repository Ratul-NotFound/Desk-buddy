#include <Arduino.h>

// ========================================================
// 🗣️ DESK BUDDY - MINI HUMAN VOICE SYNTHESIZER (Arduino Uno)
// ========================================================
// Uses vocal tract formant synthesis on Pin 11 to produce
// real phonetic human vocal sounds without any external files!
//
// Vocals included:
//   1. "HELLO!" (Cute mini robot greeting)
//   2. "YAY! / HOORAY!" (Joyful high-pitched shout)
//   3. "OH NO!" (Cute sad voice)
//   4. "WOW!" (Surprised vocal gasp)
//   5. Cute Robot Giggle / Chuckle (He-he-he!)
//   6. R2-D2 Whistles & Melodies
//
// Commands in Serial Monitor (9600 baud):
//   '1' -> Speak "HELLO!" 🗣️
//   '2' -> Speak "YAY!" 🎉
//   '3' -> Speak "OH-NO!" 🥺
//   '4' -> Speak "WOW!" 😮
//   '5' -> Giggling "HE-HE-HE" 😄
//   '6' -> R2-D2 Chirp & Melodies 🤖
//   '0' -> Auto-Cycle All Voices
// ========================================================

const int AUDIO_PIN = 11; // Connected to PAM8403 L_IN

// --------------------------------------------------------
// VOCAL TRACT FORMANT SYNTHESIZER
// --------------------------------------------------------
// Human vowels are formed by resonant frequencies (Formants F1 & F2).
// By modulating fundamental pitch (F0) and dual formant resonances,
// we synthesize real human vocal phonemes!

void speakPhoneme(int f0, int f1, int f2, int durationMs) {
    unsigned long start = millis();
    int period = 1000000 / f0; // Pitch period in microseconds
    int t1 = 500000 / f1;      // Formant 1 half-period
    int t2 = 500000 / f2;      // Formant 2 half-period

    while (millis() - start < (unsigned long)durationMs) {
        // Glottal pulse excitation
        for (int p = 0; p < period; p += (t1 + t2)) {
            digitalWrite(AUDIO_PIN, HIGH);
            delayMicroseconds(t1);
            digitalWrite(AUDIO_PIN, LOW);
            delayMicroseconds(t2);
        }
    }
}

// 1. Speak "HELLO!"
void speakHello() {
    Serial.println(F(">> Speaking: \"HELLO!\" 👋"));
    // "H" - Soft aspiration breath
    for (int i = 0; i < 60; i++) {
        digitalWrite(AUDIO_PIN, random(0, 2));
        delayMicroseconds(random(100, 350));
    }
    // "EH" - Formants F1=550Hz, F2=1800Hz, Pitch=240Hz (Friendly high tone)
    speakPhoneme(240, 550, 1800, 160);

    // "L" - Glide transition
    speakPhoneme(220, 380, 1100, 100);

    // "OH" - Formants F1=500Hz, F2=900Hz, Pitch drops slightly (210Hz)
    speakPhoneme(200, 480, 880, 240);
    digitalWrite(AUDIO_PIN, LOW);
}

// 2. Speak "YAY!"
void speakYay() {
    Serial.println(F(">> Speaking: \"YAY!\" 🎉"));
    // "Y" -> "EH" -> "EE" rising inflection
    speakPhoneme(260, 300, 2200, 80);
    speakPhoneme(320, 550, 1900, 140);
    speakPhoneme(360, 350, 2300, 180);
    digitalWrite(AUDIO_PIN, LOW);
}

// 3. Speak "OH NO!"
void speakOhNo() {
    Serial.println(F(">> Speaking: \"OH NO!\" 🥺"));
    // "OH"
    speakPhoneme(220, 500, 900, 180);
    delay(80);
    // "NO" (nasal N glide into falling O)
    speakPhoneme(240, 300, 1300, 90);
    speakPhoneme(180, 520, 850, 260);
    digitalWrite(AUDIO_PIN, LOW);
}

// 4. Speak "WOW!"
void speakWow() {
    Serial.println(F(">> Speaking: \"WOW!\" 😮"));
    // "W" glide -> "AH" -> "OO"
    speakPhoneme(190, 350, 750, 90);
    speakPhoneme(260, 750, 1200, 160); // Wide open "AH"
    speakPhoneme(210, 350, 800, 180);  // Closing "OO"
    digitalWrite(AUDIO_PIN, LOW);
}

// 5. Cute Humanoid Giggle / Laugh
void speakGiggle() {
    Serial.println(F(">> Speaking: \"HE-HE-HE!\" 😄"));
    for (int g = 0; g < 4; g++) {
        // Little breath
        for (int i = 0; i < 20; i++) {
            digitalWrite(AUDIO_PIN, random(0, 2));
            delayMicroseconds(200);
        }
        // Cute high "HEE"
        speakPhoneme(340 + (g * 20), 320, 2200, 70);
        delay(40);
    }
    digitalWrite(AUDIO_PIN, LOW);
}

// 6. R2-D2 Whistles
void soundR2D2() {
    Serial.println(F(">> Playing: R2-D2 Chirps 🤖"));
    for (int i = 0; i < 5; i++) {
        int startFreq = random(1200, 2800);
        int endFreq = startFreq + random(-700, 700);
        for (int s = 0; s < 12; s++) {
            int f = startFreq + ((endFreq - startFreq) * s / 12);
            tone(AUDIO_PIN, f, 14);
            delay(12);
        }
        delay(random(25, 60));
    }
    noTone(AUDIO_PIN);
}

// --------------------------------------------------------
// SETUP & LOOP
// --------------------------------------------------------
void setup() {
    Serial.begin(9600);
    pinMode(AUDIO_PIN, OUTPUT);
    digitalWrite(AUDIO_PIN, LOW);
    delay(500);

    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F("   🗣️ MINI HUMAN VOICE SYNTHESIZER READY "));
    Serial.println(F("=========================================="));
    Serial.println(F("Commands in Serial Monitor (9600 baud):"));
    Serial.println(F("  '1' -> Speak \"HELLO!\" 👋"));
    Serial.println(F("  '2' -> Speak \"YAY!\" 🎉"));
    Serial.println(F("  '3' -> Speak \"OH-NO!\" 🥺"));
    Serial.println(F("  '4' -> Speak \"WOW!\" 😮"));
    Serial.println(F("  '5' -> Giggle \"HE-HE-HE\" 😄"));
    Serial.println(F("  '6' -> R2-D2 Whistle 🤖"));
    Serial.println(F("  '0' -> Auto-Cycle All Voices"));
    Serial.println(F("=========================================="));

    // Greet on boot
    delay(500);
    speakHello();
}

int autoIndex = 0;
unsigned long autoTimer = 0;
bool autoMode = true;

void loop() {
    // 1. Interactive Serial Commands
    if (Serial.available() > 0) {
        char c = Serial.read();
        autoMode = false;
        switch (c) {
            case '1': speakHello(); break;
            case '2': speakYay(); break;
            case '3': speakOhNo(); break;
            case '4': speakWow(); break;
            case '5': speakGiggle(); break;
            case '6': soundR2D2(); break;
            case '0':
                Serial.println(F(">> Auto-Cycle Resumed!"));
                autoMode = true;
                autoTimer = millis();
                break;
            default: break;
        }
    }

    // 2. Auto-Cycle Mode (Speaks every 4 seconds)
    if (autoMode) {
        unsigned long now = millis();
        if (now - autoTimer >= 4000) {
            autoTimer = now;
            autoIndex = (autoIndex + 1) % 6;
            switch (autoIndex) {
                case 0: speakHello(); break;
                case 1: speakYay(); break;
                case 2: speakOhNo(); break;
                case 3: speakWow(); break;
                case 4: speakGiggle(); break;
                case 5: soundR2D2(); break;
            }
        }
    }
}
