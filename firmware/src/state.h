#pragma once

#include <Arduino.h>

enum AppState {
    STATE_CONNECTING,
    STATE_IDLE,
    STATE_ALERT,
    STATE_ERROR,      // WiFi disconnected
    STATE_API_ERROR,  // WiFi OK but API unreachable
    STATE_PORTAL
};

struct EventData {
    String title;
    String description;
    String startsAt;
    int    alertMinutesBefore;
    bool   valid;
};

// Custom colors for the cat (hex strings from API, e.g. "#FF0000")
struct CatColors {
    String body;
    String stripes;
    String belly;
    String outline;
    String eyes;
    String nose;
    String bgType;   // "solid", "stars", "sky", "sunset", "field"
    String bgColor;  // hex color for solid background
    bool   valid;
};

void  stateSetup();
void  stateUpdate();

AppState     stateGet();
EventData    stateGetEvent();

// Sync clock from server ISO timestamp
void  stateSyncTime(const String& isoTime);

// Parse and store event from JSON string (empty = no event)
void  stateSetEvent(const String& eventJson);

// Get formatted time "HH:MM"
String stateGetTimeStr();

// Get formatted date "DD/MM"
String stateGetDateStr();

// How many minutes until the event starts (negative = past)
int stateMinutesUntilEvent();

// Dismiss the current alert (won't re-trigger for this same event)
void stateDismissAlert();

// Force portal state (called when WiFi setup is needed)
void stateForcePortal();

// Set custom cat colors from JSON (called by network poll)
void stateSetColors(const String& colorsJson);

// Get current custom colors
CatColors stateGetColors();
