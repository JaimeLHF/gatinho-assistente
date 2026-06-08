#pragma once
#include <Arduino.h>

extern bool g_needsWiFiSetup;  // true quando precisa de re-configuracao

void wifiConfigBegin();
String wifiConfigGetSSID();
String wifiConfigGetPassword();
String wifiConfigGetApiUrl();
String wifiConfigGetToken();
void wifiConfigSave(const String& ssid, const String& password,
                    const String& apiUrl, const String& token);
void wifiConfigClear();
bool wifiConfigHasStored();
