#pragma once
#include <Arduino.h>

extern int g_resetPressMs;  // duracao atual do long-press (0 = solto)

void buttonResetUpdate();
