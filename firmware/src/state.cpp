#include "state.h"
#include "network.h"
#include "weather.h"
#include "config.h"

#include <ArduinoJson.h>
#include <time.h>

static AppState    currentState = STATE_CONNECTING;
static EventData   currentEvent = { "", "", "", 5, false };
static CatColors   currentColors = { "", "", "", "", "", "", "", "", false };
static unsigned long lastPollMs = 0;
static bool        firstPoll   = true;
static String      dismissedStartsAt = "";

// Time sync: offset between server time and local millis
static time_t      syncedEpoch    = 0;
static unsigned long syncedMillis = 0;

static time_t currentEpoch() {
    if (syncedEpoch == 0) return 0;
    unsigned long elapsed = (millis() - syncedMillis) / 1000;
    return syncedEpoch + (time_t)elapsed;
}

// Parse ISO 8601 "2026-05-22T20:30:00.000Z" to epoch
static time_t parseISO(const String& iso) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    // "2026-05-22T20:30:00"
    sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d",
           &t.tm_year, &t.tm_mon, &t.tm_mday,
           &t.tm_hour, &t.tm_min, &t.tm_sec);
    t.tm_year -= 1900;
    t.tm_mon  -= 1;
    return mktime(&t) - _timezone;
}

void stateSetup() {
    currentState = STATE_CONNECTING;
    setenv("TZ", "UTC0", 1);
    tzset();
}

void stateUpdate() {
    // Portal mode: skip all normal logic
    if (currentState == STATE_PORTAL) return;

    // Handle WiFi connection state
    if (!networkIsConnected()) {
        if (currentState != STATE_CONNECTING) {
            currentState = STATE_ERROR;
            Serial.println("[state] WiFi lost, switching to ERROR");
        }
        return;
    }

    if (currentState == STATE_CONNECTING || currentState == STATE_ERROR) {
        currentState = STATE_IDLE;
        Serial.println("[state] WiFi connected, switching to IDLE");
    }

    // Poll periodically (retry faster when API is down)
    unsigned long now = millis();
    unsigned long pollInterval = (currentState == STATE_API_ERROR) ? 15000 : POLL_INTERVAL_MS;
    if (firstPoll || (now - lastPollMs >= pollInterval)) {
        firstPoll = false;
        lastPollMs = now;

        String eventJson, serverTime, colorsJson;
        if (networkPoll(eventJson, serverTime, colorsJson)) {
            if (currentState == STATE_API_ERROR) {
                Serial.println("[state] API recovered");
                currentState = STATE_IDLE;
                weatherForceRefresh();
            }
            stateSyncTime(serverTime);
            stateSetEvent(eventJson);
            stateSetColors(colorsJson);
        } else {
            currentState = STATE_API_ERROR;
            return;
        }
    }

    // Determine alert state
    if (currentEvent.valid) {
        int mins = stateMinutesUntilEvent();
        if (mins >= 0 && mins <= currentEvent.alertMinutesBefore
            && currentEvent.startsAt != dismissedStartsAt) {
            currentState = STATE_ALERT;
        } else if (mins < 0) {
            // Event passed, clear it
            currentEvent.valid = false;
            dismissedStartsAt = "";
            currentState = STATE_IDLE;
        } else {
            currentState = STATE_IDLE;
        }
    } else {
        if (currentState == STATE_ALERT) {
            currentState = STATE_IDLE;
        }
        dismissedStartsAt = "";
    }
}

AppState stateGet() {
    return currentState;
}

EventData stateGetEvent() {
    return currentEvent;
}

void stateSyncTime(const String& isoTime) {
    syncedEpoch  = parseISO(isoTime);
    syncedMillis = millis();
    Serial.printf("[time] synced to epoch %ld\n", (long)syncedEpoch);
}

void stateSetEvent(const String& eventJson) {
    if (eventJson.isEmpty()) {
        currentEvent.valid = false;
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, eventJson);
    if (err) {
        currentEvent.valid = false;
        return;
    }

    currentEvent.title              = doc["title"].as<String>();
    currentEvent.description        = doc["description"].as<String>();
    currentEvent.startsAt           = doc["startsAt"].as<String>();
    currentEvent.alertMinutesBefore = doc["alertMinutesBefore"] | 5;
    currentEvent.valid              = true;
}

String stateGetTimeStr() {
    time_t t = currentEpoch();
    if (t == 0) return "--:--";
    struct tm* info = gmtime(&t);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", info->tm_hour, info->tm_min);
    return String(buf);
}

String stateGetDateStr() {
    time_t t = currentEpoch();
    if (t == 0) return "--/--";
    struct tm* info = gmtime(&t);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d/%02d", info->tm_mday, info->tm_mon + 1);
    return String(buf);
}

void stateDismissAlert() {
    if (currentEvent.valid) {
        dismissedStartsAt = currentEvent.startsAt;
        currentState = STATE_IDLE;
        Serial.println("[state] alert dismissed");
    }
}

void stateForcePortal() {
    currentState = STATE_PORTAL;
    Serial.println("[state] Entrando em STATE_PORTAL");
}

void stateSetColors(const String& colorsJson) {
    if (colorsJson.isEmpty()) {
        currentColors.valid = false;
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, colorsJson);
    if (err) {
        currentColors.valid = false;
        return;
    }

    currentColors.body    = doc["body"].as<String>();
    currentColors.stripes = doc["stripes"].as<String>();
    currentColors.belly   = doc["belly"].as<String>();
    currentColors.outline = doc["outline"].as<String>();
    currentColors.eyes    = doc["eyes"].as<String>();
    currentColors.nose    = doc["nose"].as<String>();
    currentColors.bgType  = doc["bgType"] | "solid";
    currentColors.bgColor = doc["bgColor"] | "#000000";
    currentColors.valid   = true;
    Serial.println("[state] custom colors loaded");
}

CatColors stateGetColors() {
    return currentColors;
}

int stateMinutesUntilEvent() {
    if (!currentEvent.valid) return 9999;
    time_t now   = currentEpoch();
    time_t event = parseISO(currentEvent.startsAt);
    return (int)((event - now) / 60);
}
