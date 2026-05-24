#include "network.h"
#include "config.h"
#include "wifi_config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void networkSetup() {
    wifiConfigBegin();
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiConfigGetSSID().c_str(), wifiConfigGetPassword().c_str());
    Serial.print("[wifi] connecting");
}

bool networkIsConnected() {
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
