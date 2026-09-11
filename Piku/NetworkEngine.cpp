// =========================================================================
// NetworkEngine.cpp — Robust WiFi + NTP + Weather for PIKU 2.0
// =========================================================================
#include "NetworkEngine.h"

void NetworkEngine::init(const char* apSsid, const char* apPass, const char* mdnsName) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    // Start AP for initial config / parallel access
    WiFi.softAP(apSsid, apPass);
    Serial.printf("[WiFi] AP: %s  IP: %s\n", apSsid, WiFi.softAPIP().toString().c_str());

    // mDNS so user can reach http://piku.local
    if (MDNS.begin(mdnsName)) {
        Serial.printf("[mDNS] http://%s.local ready\n", mdnsName);
    }
}

// ─── Connect to router ────────────────────────────────────────────────────────
void NetworkEngine::connectStation(const String& ssid, const String& pass, int gmtOffsetHours) {
    if (ssid.length() == 0) return;
    _staSSID     = ssid;
    _staPass     = pass;
    _gmtOffset   = gmtOffsetHours;
    _reconnectAt = millis();          // trigger immediate attempt in update()
    Serial.printf("[WiFi] Queued connection to: %s\n", ssid.c_str());
}

// ─── Must be called from networkTask every loop ───────────────────────────────
// Handles: connection, reconnect watchdog, NTP re-sync after reconnect
void NetworkEngine::update() {
    unsigned long now = millis();

    wl_status_t status = WiFi.status();
    bool connected = (status == WL_CONNECTED);

    if (connected) {
        _wasConnected = true;
        _failCount    = 0;
        _reconnectAt  = 0;   // clear scheduled reconnect

        // NTP sync once after connection
        if (!timeIsSynced) {
            syncNTP(_gmtOffset);
        }
        return;
    }

    // ---- Not connected ----
    if (_wasConnected) {
        // Just lost connection
        _wasConnected   = false;
        timeIsSynced    = false;
        weatherIsSynced = false;
        _failCount      = 0;
        _reconnectAt    = now + 3000UL;  // wait 3s then retry
        Serial.println("[WiFi] Connection lost — scheduling reconnect");
    }

    // Scheduled reconnect
    if (_reconnectAt > 0 && now >= _reconnectAt && _staSSID.length() > 0) {
        _reconnectAt = 0;
        _failCount++;
        Serial.printf("[WiFi] Reconnect attempt #%d to %s\n", _failCount, _staSSID.c_str());
        WiFi.disconnect(false);
        delay(50);
        WiFi.begin(_staSSID.c_str(), _staPass.c_str());

        // Back-off: 8s, 15s, 30s, 60s max
        unsigned long backoffMs = min(60000UL, 8000UL * (unsigned long)_failCount);
        _reconnectAt = now + backoffMs;
    }
}

// ─── Returns true ONLY once, first time we detect connection ──────────────────
bool NetworkEngine::justConnected() {
    bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected && !_reportedConnected) {
        _reportedConnected = true;
        return true;
    }
    if (!connected) _reportedConnected = false;
    return false;
}

// ─── Save creds to NVS ────────────────────────────────────────────────────────
void NetworkEngine::saveStaCreds(const String& ssid, const String& pass) {
    Preferences p; p.begin("piku", false);
    p.putString("sta_ssid", ssid);
    p.putString("sta_pass", pass);
    p.end();
    _staSSID = ssid;
    _staPass = pass;
}

// ─── NTP sync ─────────────────────────────────────────────────────────────────
void NetworkEngine::syncNTP(int gmtOffsetHours) {
    _gmtOffset = gmtOffsetHours;
    configTime(gmtOffsetHours * 3600L, 0, NTP_SERVER, "time.nist.gov", "time.google.com");
    struct tm t;
    // Wait up to 6 seconds for sync
    timeIsSynced = getLocalTime(&t, 6000);
    if (timeIsSynced) {
        Serial.println(F("[NTP] Time synchronized OK"));
    } else {
        Serial.println(F("[NTP] Sync timeout — will retry"));
    }
}

// ─── Weather fetch from Open-Meteo (plain HTTP) ───────────────────────────────
bool NetworkEngine::fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast"
                 "?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) +
                 "&current_weather=true"
                 "&hourly=relativehumidity_2m"
                 "&forecast_days=1"
                 "&timezone=auto";
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("PikuDeskBuddy/2.0");
    http.setTimeout(8000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[Weather] HTTP error: %d\n", code);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    // Parse temperature
    int ti = body.indexOf("\"temperature\":");
    if (ti != -1) {
        int end = body.indexOf(',', ti + 14);
        if (end == -1) end = body.indexOf('}', ti + 14);
        if (end != -1) *outTemp = (int)body.substring(ti + 14, end).toFloat();
    }

    // Parse weather code → condition string
    int wi = body.indexOf("\"weathercode\":");
    if (wi != -1) {
        int end = body.indexOf(',', wi + 14);
        if (end == -1) end = body.indexOf('}', wi + 14);
        int wcode = (end != -1) ? body.substring(wi + 14, end).toInt() : 0;

        if      (wcode == 0)    *outCondition = "Sunny";
        else if (wcode <= 3)    *outCondition = "Partly Cloudy";
        else if (wcode <= 48)   *outCondition = "Foggy";
        else if (wcode <= 67)   *outCondition = "Rainy";
        else if (wcode <= 77)   *outCondition = "Snowy";
        else                    *outCondition = "Stormy";
    }

    // Parse first humidity value
    int hi = body.indexOf("\"relativehumidity_2m\":[");
    if (hi != -1) {
        int start = hi + 23;
        int end   = body.indexOf(',', start);
        if (end == -1) end = body.indexOf(']', start);
        if (end != -1) *outHumidity = body.substring(start, end).toInt();
    }

    weatherIsSynced = true;
    Serial.printf("[Weather] %d°C, %s, Hum: %d%%\n", *outTemp, outCondition->c_str(), *outHumidity);
    return true;
}

// ─── Time formatters ──────────────────────────────────────────────────────────
String NetworkEngine::getFormattedTime() {
    struct tm t;
    if (!getLocalTime(&t, 100)) return "--:--";
    char buf[12];
    strftime(buf, sizeof(buf), "%I:%M %p", &t);
    return String(buf);
}

String NetworkEngine::getFormattedDate() {
    struct tm t;
    if (!getLocalTime(&t, 100)) return "---";
    char buf[24];
    strftime(buf, sizeof(buf), "%a, %d %b %Y", &t);
    return String(buf);
}

// ─── WiFi scan ────────────────────────────────────────────────────────────────
String NetworkEngine::scanNetworks() {
    int n = WiFi.scanNetworks(false, false, false, 300);
    String json = "[";
    for (int i = 0; i < n && i < 20; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    WiFi.scanDelete();
    return json;
}

String NetworkEngine::getStaIP() {
    return WiFi.localIP().toString();
}

String NetworkEngine::getApIP() {
    return WiFi.softAPIP().toString();
}

bool NetworkEngine::isStaConnected() {
    return WiFi.status() == WL_CONNECTED;
}
