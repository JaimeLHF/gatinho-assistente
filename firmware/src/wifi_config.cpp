#include "wifi_config.h"
#include <Preferences.h>
#include "config.h"

static Preferences prefs;
static const char* NS = "wifi";

void wifiConfigBegin() {
    prefs.begin(NS, false);
    if (!prefs.isKey("ssid")) {
        Serial.println("[wifi-config] NVS vazio, usando config.h fallback");
        prefs.putString("ssid", WIFI_SSID);
        prefs.putString("pwd", WIFI_PASSWORD);
    } else {
        Serial.println("[wifi-config] Lendo SSID/pwd do NVS");
    }
}

String wifiConfigGetSSID() {
    return prefs.getString("ssid", WIFI_SSID);
}

String wifiConfigGetPassword() {
    return prefs.getString("pwd", WIFI_PASSWORD);
}

void wifiConfigSave(const String& ssid, const String& password) {
    prefs.putString("ssid", ssid);
    prefs.putString("pwd", password);
    Serial.printf("[wifi-config] Salvo SSID=%s\n", ssid.c_str());
}

void wifiConfigClear() {
    prefs.clear();
    Serial.println("[wifi-config] NVS limpo");
}

bool wifiConfigHasStored() {
    return prefs.isKey("ssid");
}
