#include "time_sync.h"
#include <time.h>

static bool synced = false;

static const char* DAYS[]   = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};
static const char* MONTHS[] = {"Jan","Fev","Mar","Abr","Mai","Jun",
                               "Jul","Ago","Set","Out","Nov","Dez"};

void timeSyncInit() {
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");
    Serial.println("[ntp] configTime called (GMT-3)");
}

bool timeSyncIsReady() {
    if (synced) return true;
    struct tm ti;
    if (getLocalTime(&ti, 0) && ti.tm_year > 100) {
        synced = true;
        Serial.println("[ntp] synced");
    }
    return synced;
}

String timeSyncGetHHMM() {
    struct tm ti;
    if (!getLocalTime(&ti, 0)) return "--:--";
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
    return String(buf);
}

String timeSyncGetDateStr() {
    struct tm ti;
    if (!getLocalTime(&ti, 0)) return "";
    char buf[16];
    snprintf(buf, sizeof(buf), "%s, %02d %s",
             DAYS[ti.tm_wday], ti.tm_mday, MONTHS[ti.tm_mon]);
    return String(buf);
}
