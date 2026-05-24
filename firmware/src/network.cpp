#include "network.h"
#include "config.h"
#include "wifi_config.h"
#include "render.h"
#include "state.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void networkSetup() {
    wifiConfigBegin();
    WiFi.mode(WIFI_STA);
    String ssid = wifiConfigGetSSID();
    String pwd  = wifiConfigGetPassword();
    WiFi.begin(ssid.c_str(), pwd.c_str());
    Serial.printf("[wifi] connecting to '%s'", ssid.c_str());

    unsigned long startMs = millis();
    unsigned long lastDotMs = 0;
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - startMs > 25000) {
            Serial.println("\n[wifi] Timeout 25s, marcando needsWiFiSetup");
            g_needsWiFiSetup = true;
            break;
        }
        if (now - lastDotMs >= 500) {
            Serial.print(".");
            lastDotMs = now;
        }
        renderFrame(STATE_CONNECTING);
        delay(33);
    }

    if (WiFi.status() == WL_CONNECTED) {
        g_needsWiFiSetup = false;
        Serial.printf("\n[wifi] Conectado em %lums, IP: %s\n",
                       millis() - startMs, WiFi.localIP().toString().c_str());
    }
}

bool networkIsConnected() {
    // Se reconectou depois do timeout, limpa a flag
    if (g_needsWiFiSetup && WiFi.status() == WL_CONNECTED) {
        g_needsWiFiSetup = false;
        Serial.println("[wifi] Reconectou depois do timeout");
    }
    return WiFi.status() == WL_CONNECTED;
}

bool networkPoll(String& eventJson, String& serverTime) {
    if (!networkIsConnected()) return false;

    HTTPClient http;
    String url = String(API_BASE_URL) + "/device/next-event";
    http.begin(url);
    http.addHeader("X-Device-Token", DEVICE_TOKEN);
    http.setTimeout(10000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[http] poll failed, code=%d\n", code);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[json] parse error: %s\n", err.c_str());
        return false;
    }

    serverTime = doc["currentTime"].as<String>();

    if (doc["event"].is<JsonObject>()) {
        String buf;
        serializeJson(doc["event"], buf);
        eventJson = buf;
    } else {
        eventJson = "";
    }

    Serial.printf("[poll] ok, event=%s\n", eventJson.isEmpty() ? "none" : "yes");
    return true;
}
