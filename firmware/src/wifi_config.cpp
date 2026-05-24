#include "wifi_config.h"
#include <Preferences.h>
#include "config.h"

bool g_needsWiFiSetup = false;

static Preferences prefs;
static const char* NS = "wifi";

void wifiConfigBegin() {
    prefs.begin(NS, false);
    if (prefs.getBool("cleared", false)) {
        // Reset intencional pelo usuario — vai direto pro portal
        prefs.remove("cleared");
        Serial.println("[wifi-config] Reset pelo usuario, entrando em modo portal");
        return;
    }
    if (!prefs.isKey("ssid")) {
        // Primeiro boot — semeia com config.h
        Serial.println("[wifi-config] NVS vazio, usando config.h fallback");
        prefs.putString("ssid", WIFI_SSID);
        prefs.putString("pwd", WIFI_PASSWORD);
    } else {
        Serial.println("[wifi-config] Lendo SSID/pwd do NVS");
    }
}

String wifiConfigGetSSID() {
    return prefs.getString("ssid", "");
}

String wifiConfigGetPassword() {
    return prefs.getString("pwd", "");
}

void wifiConfigSave(const String& ssid, const String& password) {
    prefs.putString("ssid", ssid);
    prefs.putString("pwd", password);
    Serial.printf("[wifi-config] Salvo SSID=%s\n", ssid.c_str());
}

void wifiConfigClear() {
    prefs.clear();
    prefs.putBool("cleared", true);
    Serial.println("[wifi-config] NVS limpo, flag de reset salva");
}

bool wifiConfigHasStored() {
    return prefs.isKey("ssid");
}
