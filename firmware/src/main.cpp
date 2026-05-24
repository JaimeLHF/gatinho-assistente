#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "network.h"
#include "state.h"
#include "render.h"
#include "time_sync.h"
#include "wifi_config.h"
#include "wifi_portal.h"
#include "button_reset.h"

static bool ntpStarted = false;

static unsigned long lastRenderMs = 0;
static const unsigned long RENDER_INTERVAL_MS = 33; // ~30fps for smooth animations

void setup() {
    Serial.begin(115200);
    Serial.println("\n[gatinho-pet] booting...");

    renderSetup();
    stateSetup();
    renderFrame(stateGet());   // mostra tela "Conectando" antes de bloquear
    networkSetup();

    if (g_needsWiFiSetup) {
        // Mostra erro com countdown de 10s, mas checa se WiFi conecta nesse tempo
        unsigned long errStart = millis();
        while (millis() - errStart < 10000) {
            buttonResetUpdate();
            if (WiFi.status() == WL_CONNECTED) {
                g_needsWiFiSetup = false;
                Serial.println("[wifi] Conectou durante countdown, pulando portal");
                break;
            }
            int secsLeft = (int)((10000 - (millis() - errStart)) / 1000) + 1;
            renderWifiRetry(secsLeft);
            delay(33);
        }
        if (g_needsWiFiSetup) {
            portalStart();
            stateForcePortal();
        }
    }
}

void loop() {
    buttonResetUpdate();

    if (portalIsRunning()) {
        portalLoop();
    } else {
        stateUpdate();

        // Init NTP once WiFi is connected
        if (!ntpStarted && networkIsConnected()) {
            timeSyncInit();
            ntpStarted = true;
        }
    }

    unsigned long now = millis();
    if (now - lastRenderMs >= RENDER_INTERVAL_MS) {
        lastRenderMs = now;
        renderFrame(stateGet());
    }
}
