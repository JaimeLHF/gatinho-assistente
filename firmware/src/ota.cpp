#include "ota.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>

static WebServer otaServer(8080);
static bool otaRunning = false;

void otaSetup() {
    if (otaRunning) return;

    otaServer.on("/", HTTP_GET, []() {
        otaServer.send(200, "text/html",
            "<h2>Gatinho Pet</h2>"
            "<p>IP: " + WiFi.localIP().toString() + "</p>"
            "<p><a href='/update'>Atualizar Firmware</a></p>");
    });

    ElegantOTA.begin(&otaServer);
    otaServer.begin();
    otaRunning = true;

    Serial.printf("[ota] Web OTA disponivel em http://%s:8080/update\n",
                  WiFi.localIP().toString().c_str());
}

void otaLoop() {
    if (!otaRunning) return;
    otaServer.handleClient();
    ElegantOTA.loop();
}

void otaStop() {
    if (!otaRunning) return;
    otaServer.stop();
    otaRunning = false;
    Serial.println("[ota] Parado");
}
