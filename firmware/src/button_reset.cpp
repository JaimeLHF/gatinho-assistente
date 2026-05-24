#include "button_reset.h"
#include "wifi_config.h"

static const int BTN_BOOT = 0;
static const unsigned long RESET_HOLD_MS = 10000;

int g_resetPressMs = 0;

static unsigned long bootPressStartMs = 0;

void buttonResetUpdate() {
    bool down = (digitalRead(BTN_BOOT) == LOW);

    if (down) {
        if (bootPressStartMs == 0) {
            bootPressStartMs = millis();
        }
        unsigned long held = millis() - bootPressStartMs;
        g_resetPressMs = (int)held;

        if (held >= RESET_HOLD_MS) {
            Serial.println("[reset] Long press detectado, limpando NVS");
            wifiConfigClear();
            delay(500);
            ESP.restart();
        }
    } else {
        bootPressStartMs = 0;
        g_resetPressMs = 0;
    }
}
