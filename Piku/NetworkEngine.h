#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"

class NetworkEngine {
public:
    void   init(const char* apSsid, const char* apPass, const char* mdnsName);
    void   connectStation(const String& ssid, const String& pass, int gmtOffsetHours);
    bool   isStaConnected() const { return WiFi.status() == WL_CONNECTED; }
    String getStaIP()       const { return WiFi.localIP().toString(); }
    String getApIP()        const { return WiFi.softAPIP().toString(); }
    void   saveStaCreds(const String& ssid, const String& pass);

    void   syncNTP(int gmtOffsetHours);
    void   fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition);
    String getFormattedTime();
    String getFormattedDate();
    String scanNetworks();

    bool   timeIsSynced    = false;
    bool   weatherIsSynced = false;

private:
    int _gmtOffset = DEFAULT_GMT_OFFSET;
};
