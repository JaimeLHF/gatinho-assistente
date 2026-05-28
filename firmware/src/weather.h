#pragma once

#include <Arduino.h>

enum WeatherIcon {
    ICON_CLEAR,          // WMO 0-1
    ICON_PARTLY_CLOUDY,  // WMO 2-3
    ICON_CLOUDY,         // WMO 45, 48
    ICON_RAIN,           // WMO 51-67, 80-82, 95-99
    ICON_SNOW            // WMO 71-77, 85-86
};

// Call from loop(); handles its own 10-min polling interval.
void weatherUpdate();

bool weatherIsReady();
int  weatherGetTemp();           // rounded integer
WeatherIcon weatherGetIcon();
void weatherForceRefresh();      // reset timer to fetch on next update
