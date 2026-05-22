#pragma once

#include <Arduino.h>

enum AppState {
    STATE_CONNECTING,
    STATE_IDLE,
    STATE_ALERT,
    STATE_ERROR
};

struct EventData {
    String title;
    String startsAt;
    int    alertMinutesBefore;
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
