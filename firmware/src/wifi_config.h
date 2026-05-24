#pragma once
#include <Arduino.h>

void wifiConfigBegin();
String wifiConfigGetSSID();
String wifiConfigGetPassword();
void wifiConfigSave(const String& ssid, const String& password);
void wifiConfigClear();
bool wifiConfigHasStored();
