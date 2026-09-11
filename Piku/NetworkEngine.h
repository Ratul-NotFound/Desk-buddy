#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"

class NetworkEngine {
public:
    bool timeIsSynced    = false;
    bool weatherIsSynced = false;

    void   init(const char* apSsid, const char* apPass, const char* mdnsName);
    void   update();    // call from networkTask every loop — handles reconnect watchdog

    void   connectStation(const String& ssid, const String& pass, int gmtOffsetHours);
    void   saveStaCreds(const String& ssid, const String& pass);
    void   syncNTP(int gmtOffsetHours);
    bool   fetchWeather(float lat, float lon, int* outTemp, int* outHumidity, String* outCondition);

    // Returns TRUE exactly once when STA first connects (edge detect)
    bool   justConnected();

    bool   isStaConnected();
    String getStaIP();
    String getApIP();
    String getFormattedTime();
    String getFormattedDate();
    String scanNetworks();

private:
    String _staSSID, _staPass;
    int    _gmtOffset = 6;

    bool          _wasConnected      = false;
    bool          _reportedConnected = false;
    int           _failCount         = 0;
    unsigned long _reconnectAt       = 0;
};
