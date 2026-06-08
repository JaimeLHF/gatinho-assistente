#include "auto_update.h"
#include "config.h"
#include "wifi_config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

static WiFiClientSecure secureClient;
static WiFiClient plainClient;

static unsigned long lastCheckMs = 0;
static const unsigned long CHECK_INTERVAL_MS = 300000;  // check every 5 minutes
static bool firstCheck = true;

static void httpBegin(HTTPClient& http, const String& url) {
    if (url.startsWith("https")) {
        secureClient.setInsecure();
        http.begin(secureClient, url);
    } else {
        http.begin(plainClient, url);
    }
}

void autoUpdateCheck() {
    if (WiFi.status() != WL_CONNECTED) return;

    unsigned long now = millis();
    if (!firstCheck && (now - lastCheckMs < CHECK_INTERVAL_MS)) return;
    firstCheck = false;
    lastCheckMs = now;

    Serial.printf("[ota-auto] checking for updates (current: %s)\n", FIRMWARE_VERSION);

    String apiUrl = wifiConfigGetApiUrl();
    String token  = wifiConfigGetToken();

    // Step 1: Check if update is available
    HTTPClient http;
    String checkUrl = apiUrl + "/device/firmware/check";
    httpBegin(http, checkUrl);
    http.addHeader("X-Device-Token", token);
    http.addHeader("X-Firmware-Version", FIRMWARE_VERSION);
    http.setTimeout(10000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[ota-auto] check failed, HTTP %d\n", code);
        http.end();
        return;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
        Serial.println("[ota-auto] JSON parse error");
        return;
    }

    bool hasUpdate = doc["update"] | false;
    if (!hasUpdate) {
        Serial.println("[ota-auto] firmware is up to date");
        return;
    }

    const char* newVersion = doc["version"];
    const char* downloadUrl = doc["url"];
    int fileSize = doc["size"] | 0;

    if (!newVersion || !downloadUrl) {
        Serial.println("[ota-auto] missing version or url in response");
        return;
    }

    Serial.printf("[ota-auto] update available: %s -> %s (%d bytes)\n",
                  FIRMWARE_VERSION, newVersion, fileSize);
    Serial.printf("[ota-auto] downloading from: %s\n", downloadUrl);

    // Step 2: Download and flash
    HTTPClient dlHttp;
    httpBegin(dlHttp, String(downloadUrl));
    dlHttp.addHeader("X-Device-Token", token);
    dlHttp.setTimeout(30000);

    int dlCode = dlHttp.GET();
    if (dlCode != 200) {
        Serial.printf("[ota-auto] download failed, HTTP %d\n", dlCode);
        dlHttp.end();
        return;
    }

    int contentLength = dlHttp.getSize();
    if (contentLength <= 0) {
        Serial.println("[ota-auto] invalid content length");
        dlHttp.end();
        return;
    }

    if (!Update.begin(contentLength)) {
        Serial.printf("[ota-auto] not enough space: %d bytes needed\n", contentLength);
        dlHttp.end();
        return;
    }

    WiFiClient* stream = dlHttp.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    dlHttp.end();

    if (written != (size_t)contentLength) {
        Serial.printf("[ota-auto] write mismatch: %d/%d\n", written, contentLength);
        Update.abort();
        return;
    }

    if (!Update.end()) {
        Serial.printf("[ota-auto] update error: %s\n", Update.errorString());
        return;
    }

    Serial.printf("[ota-auto] update success! Rebooting to v%s...\n", newVersion);
    delay(500);
    ESP.restart();
}
