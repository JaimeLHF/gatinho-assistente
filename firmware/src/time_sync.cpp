#include "time_sync.h"
#include <time.h>

static bool synced = false;

static const char* DAYS[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* MONTHS[] = {"Jan","Feb","Mar","Apr","May","Jun",
                               "Jul","Aug","Sep","Oct","Nov","Dec"};

// Cache para evitar flicker quando getLocalTime falha intermitentemente
static char cachedHHMM[9] = "--:--";   // "12:30PM"
static char cachedDate[16] = "";
static unsigned long lastUpdateMs = 0;
static const unsigned long UPDATE_INTERVAL_MS = 1000;  // atualiza 1x/s

static void updateCache() {
    unsigned long now = millis();
    if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;
    lastUpdateMs = now;

    struct tm ti;
    if (!getLocalTime(&ti, 0)) return;  // falhou, mantém cache

    int h12 = ti.tm_hour % 12;
    if (h12 == 0) h12 = 12;
    const char* ampm = (ti.tm_hour < 12) ? "AM" : "PM";
    snprintf(cachedHHMM, sizeof(cachedHHMM), "%d:%02d%s", h12, ti.tm_min, ampm);
    snprintf(cachedDate, sizeof(cachedDate), "%s, %s %d",
             DAYS[ti.tm_wday], MONTHS[ti.tm_mon], ti.tm_mday);
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
        int h12 = ti.tm_hour % 12;
        if (h12 == 0) h12 = 12;
        const char* ampm = (ti.tm_hour < 12) ? "AM" : "PM";
        snprintf(cachedHHMM, sizeof(cachedHHMM), "%d:%02d%s", h12, ti.tm_min, ampm);
        snprintf(cachedDate, sizeof(cachedDate), "%s, %s %d",
                 DAYS[ti.tm_wday], MONTHS[ti.tm_mon], ti.tm_mday);
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
