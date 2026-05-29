#pragma once

#include <Arduino.h>

// Check for firmware updates via API. Call periodically (e.g. every 5 min).
// If a new version is available, downloads and installs it, then reboots.
void autoUpdateCheck();
