#include "NetworkEngine.h"

void NetworkEngine::init(const char* apSsid, const char* apPass, const char* mdnsName) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid, apPass);
    Serial.printf("[WiFi] AP: %s  IP: %s\n", apSsid, WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(mdnsName)) {
        Serial.printf("[mDNS] http://%s.local\n", mdnsName);
    }
}

void NetworkEngine::connectStation(const String& ssid, const String& pass, int gmtOffsetHours) {
    _gmtOffset = gmtOffsetHours;
    WiFi.begin(ssid.c_str(), pass.c_str());
}

void NetworkEngine::saveStaCreds(const String& ssid, const String& pass) {
    Preferences p; p.begin("piku", false);
    p.putString("sta_ssid", ssid);
    p.putString("sta_pass", pass);
    p.end();
}

void NetworkEngine::syncNTP(int gmtOffsetHours) {
    _gmtOffset = gmtOffsetHours;
    configTime(gmtOffsetHours * 3600, 0, NTP_SERVER, "time.nist.gov", "time.google.com");
    struct tm t; timeIsSynced = getLocalTime(&t);
    if (timeIsSynced) Serial.println(F("[NTP] Time synced"));
}

void NetworkEngine::fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition) {
    if (WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) +
                 "&current_weather=true&hourly=relativehumidity_2m&forecast_days=1&timezone=auto";
    http.begin(url);
    if (http.GET() == 200) {
        String body = http.getString();
        int ti = body.indexOf("\"temperature\":");
        if (ti != -1) *outTemp = (int)body.substring(ti + 14, body.indexOf(',', ti)).toFloat();
        int wi = body.indexOf("\"weathercode\":");
        int code = (wi != -1) ? body.substring(wi + 14, body.indexOf(',', wi)).toInt() : 0;
        if      (code == 0)          *outCondition = "Sunny";
        else if (code <= 3)          *outCondition = "Partly Cloudy";
        else if (code <= 48)         *outCondition = "Foggy";
        else if (code <= 67)         *outCondition = "Rainy";
        else if (code <= 77)         *outCondition = "Snowy";
        else                         *outCondition = "Stormy";
        int hi = body.indexOf("\"relativehumidity_2m\":[");
        if (hi != -1) {
            int start = hi + 23;
            *outHumidity = body.substring(start, body.indexOf(',', start)).toInt();
        }
        weatherIsSynced = true;
        Serial.printf("[Weather] %d°C %s\n", *outTemp, outCondition->c_str());
    }
    http.end();
}

String NetworkEngine::getFormattedTime() {
    struct tm t;
    if (!getLocalTime(&t)) return "--:--";
    char buf[10];
    strftime(buf, sizeof(buf), "%I:%M %p", &t);
    return String(buf);
}

String NetworkEngine::getFormattedDate() {
    struct tm t;
    if (!getLocalTime(&t)) return "---";
    char buf[22];
    strftime(buf, sizeof(buf), "%a, %d %b %Y", &t);
    return String(buf);
}

String NetworkEngine::scanNetworks() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    return json;
}
