#include <Arduino.h>
#include "config.h"
#include "network.h"
#include "state.h"
#include "render.h"

static unsigned long lastRenderMs = 0;
static const unsigned long RENDER_INTERVAL_MS = 33; // ~30fps for smooth animations

void setup() {
    Serial.begin(115200);
    Serial.println("\n[gatinho-pet] booting...");

    renderSetup();
    stateSetup();
    networkSetup();

    renderFrame(stateGet());
}

void loop() {
    stateUpdate();

    unsigned long now = millis();
    if (now - lastRenderMs >= RENDER_INTERVAL_MS) {
        lastRenderMs = now;
        renderFrame(stateGet());
    }
}
