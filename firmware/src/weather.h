#pragma once

#include <Arduino.h>

// Call from loop(); handles its own 10-min polling interval.
void weatherUpdate();

bool weatherIsReady();
String weatherGetTempStr();  // e.g. "22*C" or "--"
