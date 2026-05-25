#include "time_sync.h"
#include <time.h>

static bool synced = false;

static const char* DAYS[]   = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};
static const char* MONTHS[] = {"Jan","Fev","Mar","Abr","Mai","Jun",
                               "Jul","Ago","Set","Out","Nov","Dez"};

// Cache para evitar flicker quando getLocalTime falha intermitentemente
static char cachedHHMM[6] = "--:--";
static char cachedDate[16] = "";
static unsigned long lastUpdateMs = 0;
static const unsigned long UPDATE_INTERVAL_MS = 1000;  // atualiza 1x/s

static void updateCache() {
    unsigned long now = millis();
    if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;
    lastUpdateMs = now;

    struct tm ti;
    if (!getLocalTime(&ti, 0)) return;  // falhou, mantém cache

    snprintf(cachedHHMM, sizeof(cachedHHMM), "%02d:%02d", ti.tm_hour, ti.tm_min);
    snprintf(cachedDate, sizeof(cachedDate), "%s, %02d %s",
             DAYS[ti.tm_wday], ti.tm_mday, MONTHS[ti.tm_mon]);
}

void timeSyncInit() {
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");
    Serial.println("[ntp] configTime called (GMT-3)");
}

bool timeSyncIsReady() {
    if (synced) return true;
    struct tm ti;
    if (getLocalTime(&ti, 0) && ti.tm_year > 100) {
        synced = true;
        snprintf(cachedHHMM, sizeof(cachedHHMM), "%02d:%02d", ti.tm_hour, ti.tm_min);
        snprintf(cachedDate, sizeof(cachedDate), "%s, %02d %s",
                 DAYS[ti.tm_wday], ti.tm_mday, MONTHS[ti.tm_mon]);
        Serial.println("[ntp] synced");
    }
    return synced;
}

String timeSyncGetHHMM() {
    updateCache();
    return String(cachedHHMM);
}

String timeSyncGetDateStr() {
    updateCache();
    return String(cachedDate);
}
