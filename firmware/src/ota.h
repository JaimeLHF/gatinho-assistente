#pragma once

#include <Arduino.h>

void otaSetup();   // call after WiFi connects
void otaLoop();    // call every loop iteration
void otaStop();    // call before switching to AP mode
