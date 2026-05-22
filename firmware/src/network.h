#pragma once

#include <Arduino.h>

void networkSetup();
bool networkIsConnected();

// Poll the API. Returns true if got a valid response.
// On success, fills eventJson (empty string if no event) and serverTime.
bool networkPoll(String& eventJson, String& serverTime);
