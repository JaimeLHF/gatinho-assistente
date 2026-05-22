#pragma once

// Copy this file to config.h and fill in your values.
// config.h is gitignored — never commit secrets.

#define WIFI_SSID     "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// API base URL (no trailing slash)
#define API_BASE_URL  "http://192.168.1.100:3001/api/v1"

// Device token from POST /api/v1/devices
#define DEVICE_TOKEN  "your-64-char-hex-device-token"

// Polling interval in milliseconds
#define POLL_INTERVAL_MS 60000
