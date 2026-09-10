#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <time.h>
#include <HTTPClient.h>
#include "config.h"

class NetworkEngine {
public:
    void init(const char* apSsid, const char* apPass, const char* mdnsName);
    void connectStation(const String& ssid, const String& pass, int gmtOffsetHours);
    void saveStaCreds(const String& ssid, const String& pass);
    bool isStaConnected() const { return WiFi.status() == WL_CONNECTED; }
    String getStaIP()     const { return WiFi.localIP().toString(); }
    String getApIP()      const { return WiFi.softAPIP().toString(); }

    void syncNTP(int gmtOffsetHours);
    void fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition);

    String getFormattedTime();
    String getFormattedDate();

    String scanNetworks();   // returns JSON array string

    bool timeIsSynced    = false;
    bool weatherIsSynced = false;

private:
    int _gmtOffset = 6;
};
