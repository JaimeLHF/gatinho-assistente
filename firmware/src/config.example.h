#pragma once

// Copy this file to config.h and fill in your values.
// config.h is gitignored — never commit secrets.
//
// These values are used as DEFAULTS on first boot only.
// After setup, all settings are stored in NVS and configured via the captive portal.
// To reconfigure, hold BOOT button for 10 seconds.

#define WIFI_SSID     "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// API base URL (no trailing slash) — pre-filled in portal, change only for local dev
#define API_BASE_URL  "https://gatinho-api.onrender.com/api/v1"

// Device token — leave empty, user will enter via portal after creating a device on the web
#define DEVICE_TOKEN  ""

// Polling interval in milliseconds
#define POLL_INTERVAL_MS 60000

// Firmware version (used for OTA auto-update check)
#define FIRMWARE_VERSION "1.0.0"

// Coordinates for weather (Open-Meteo API)
#define WEATHER_LAT  -23.5505  // São Paulo
#define WEATHER_LON  -46.6340
