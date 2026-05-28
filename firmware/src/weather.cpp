#include "weather.h"
#include "config.h"
#include "network.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

static float currentTemp = 0;
static int   weatherCode = 0;
static bool  tempValid   = false;
static unsigned long lastFetchMs = 0;
static bool firstFetch = true;
static const unsigned long WEATHER_INTERVAL_MS = 600000;  // 10 minutes

static WeatherIcon mapWmoCode(int code) {
    if (code <= 1)  return ICON_CLEAR;
    if (code <= 3)  return ICON_PARTLY_CLOUDY;
    if (code <= 48) return ICON_CLOUDY;          // fog
    if (code <= 67) return ICON_RAIN;            // drizzle + rain
    if (code <= 77) return ICON_SNOW;            // snow
    if (code <= 82) return ICON_RAIN;            // rain showers
    if (code <= 86) return ICON_SNOW;            // snow showers
    return ICON_RAIN;                            // thunderstorm
}

void weatherUpdate() {
    if (!networkIsConnected()) return;

    unsigned long now = millis();
    unsigned long cooldown = tempValid ? WEATHER_INTERVAL_MS : 15000;
    if (!firstFetch && (now - lastFetchMs < cooldown)) return;

    firstFetch = false;
    lastFetchMs = now;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
                 + String(WEATHER_LAT, 4) + "&longitude=" + String(WEATHER_LON, 4)
                 + "&current=temperature_2m,weather_code";

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
    weatherCode = doc["current"]["weather_code"] | 0;
    tempValid = true;
    Serial.printf("[weather] temp=%.1fC code=%d\n", currentTemp, weatherCode);
}

bool weatherIsReady() {
    return tempValid;
}

int weatherGetTemp() {
    return (int)roundf(currentTemp);
}

void weatherForceRefresh() {
    lastFetchMs = 0;
    firstFetch = true;
}

WeatherIcon weatherGetIcon() {
    return mapWmoCode(weatherCode);
}
