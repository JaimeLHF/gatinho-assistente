#pragma once
#include <Arduino.h>

void portalStart();
void portalStop();
bool portalIsRunning();
void portalLoop();
String portalGetAPSSID();
String portalGetAPPassword();
String portalGetAPIP();
