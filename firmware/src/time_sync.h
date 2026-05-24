#pragma once

#include <Arduino.h>

void timeSyncInit();
bool timeSyncIsReady();
String timeSyncGetHHMM();
String timeSyncGetDateStr();
