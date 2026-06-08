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
        prefs.putString("api_url", API_BASE_URL);
        prefs.putString("token", DEVICE_TOKEN);
    } else {
        Serial.println("[wifi-config] Lendo config do NVS");
        // Migrar dispositivos antigos que nao tinham api_url/token
        if (!prefs.isKey("api_url")) {
            prefs.putString("api_url", API_BASE_URL);
            prefs.putString("token", DEVICE_TOKEN);
        }
    }
}

String wifiConfigGetSSID() {
    return prefs.getString("ssid", "");
}

String wifiConfigGetPassword() {
    return prefs.getString("pwd", "");
}

String wifiConfigGetApiUrl() {
    return prefs.getString("api_url", API_BASE_URL);
}

String wifiConfigGetToken() {
    return prefs.getString("token", "");
}

void wifiConfigSave(const String& ssid, const String& password,
                    const String& apiUrl, const String& token) {
    prefs.putString("ssid", ssid);
    prefs.putString("pwd", password);
    prefs.putString("api_url", apiUrl);
    prefs.putString("token", token);
    Serial.printf("[wifi-config] Salvo SSID=%s, API=%s\n", ssid.c_str(), apiUrl.c_str());
}

void wifiConfigClear() {
    prefs.clear();
    prefs.putBool("cleared", true);
    Serial.println("[wifi-config] NVS limpo, flag de reset salva");
}

bool wifiConfigHasStored() {
    return prefs.isKey("ssid");
}
