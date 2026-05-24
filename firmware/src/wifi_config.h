#pragma once
#include <Arduino.h>

extern bool g_needsWiFiSetup;  // true quando precisa de re-configuracao

void wifiConfigBegin();
String wifiConfigGetSSID();
String wifiConfigGetPassword();
void wifiConfigSave(const String& ssid, const String& password);
void wifiConfigClear();
bool wifiConfigHasStored();
