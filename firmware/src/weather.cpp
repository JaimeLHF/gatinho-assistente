#include "weather.h"
#include "config.h"
#include "network.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

static float currentTemp = 0;
static bool  tempValid   = false;
static unsigned long lastFetchMs = 0;
static bool firstFetch = true;
static const unsigned long WEATHER_INTERVAL_MS = 600000;  // 10 minutes

void weatherUpdate() {
    if (!networkIsConnected()) return;

    unsigned long now = millis();
    unsigned long cooldown = tempValid ? WEATHER_INTERVAL_MS : 60000;  // retry after 60s on failure
    if (!firstFetch && (now - lastFetchMs < cooldown)) return;

    firstFetch = false;
    lastFetchMs = now;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
                 + String(WEATHER_LAT, 4) + "&longitude=" + String(WEATHER_LON, 4)
                 + "&current=temperature_2m";

    http.begin(client, url);
    http.setTimeout(10000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[weather] fetch failed, code=%d\n", code);
        http.end();
        return;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
        Serial.println("[weather] json parse error");
        return;
    }

    currentTemp = doc["current"]["temperature_2m"].as<float>();
    tempValid = true;
    Serial.printf("[weather] temp=%.1fC\n", currentTemp);
}

bool weatherIsReady() {
    return tempValid;
}

String weatherGetTempStr() {
    if (!tempValid) return "--";
    int t = (int)roundf(currentTemp);
    return String(t) + "C";
}
