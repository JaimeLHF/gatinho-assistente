/*
 * Gatinho Pet — Firmware completo
 * Board: LilyGo T-Display-S3 (ESP32-S3)
 *
 * Arduino IDE setup:
 * 1. Instale as libs: "TFT_eSPI" e "ArduinoJson" pelo Library Manager
 * 2. Em User_Setup_Select.h da TFT_eSPI, descomente Setup206_LilyGo_T_Display_S3
 * 3. Board: "ESP32S3 Dev Module"
 *    USB CDC On Boot: Enabled | Flash Size: 16MB | PSRAM: OPI PSRAM
 *
 * Funcionalidades:
 * - Conecta WiFi e faz polling da API a cada 60s
 * - Sincroniza relogio do servidor
 * - Mostra hora, data, gatinho pixel art
 * - Alerta visual quando evento esta proximo
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>

// =============================================================
// >>>  CONFIGURE AQUI  <<<
// =============================================================
#define WIFI_SSID      "SEU_WIFI"
#define WIFI_PASSWORD  "SUA_SENHA"
#define API_BASE_URL   "http://192.168.1.100:3001/api/v1"
#define DEVICE_TOKEN   "SEU_TOKEN_DE_64_CARACTERES_HEX"
#define POLL_INTERVAL_MS 60000
// =============================================================

// ---- Display ----
TFT_eSPI tft;
TFT_eSprite sprite(&tft);

#define W 170
#define H 320
#define CX (W / 2)

// ---- States ----
enum AppState { STATE_CONNECTING, STATE_IDLE, STATE_ALERT, STATE_ERROR };

struct EventData {
    String title;
    String startsAt;
    int    alertMinutesBefore;
    bool   valid;
};

static AppState    currentState = STATE_CONNECTING;
static EventData   currentEvent = { "", "", 5, false };
static unsigned long lastPollMs  = 0;
static bool        firstPoll    = true;
static time_t      syncedEpoch  = 0;
static unsigned long syncedMillis = 0;

// ---- Render timing ----
static unsigned long lastRenderMs = 0;

// ========== PALETA RGB565 ==========
#define BG_COLOR    0x1926
#define COL_BLACK   0x0000
#define COL_BODY    0xE504
#define COL_STRIPE  0xB380
#define COL_CREAM   0xF71B
#define COL_PINK    0xEB2D
#define TEXT_COLOR  0xFFFF
#define TEXT_DIM    0x9CF3
#define ALERT_COLOR 0xFD20
#define ALERT_BG    0x6180

const uint16_t PALETTE[] = { BG_COLOR, COL_BLACK, COL_BODY, COL_STRIPE, COL_CREAM, COL_PINK };

// ========== PIXEL ART: 16x21 ==========
#define CAT_W 16
#define CAT_H 21
#define SCALE 5

// Idle — olhos relaxados
const uint8_t CAT_IDLE[21][16] = {
    {0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,1,2,2,1,0,0,0,0,1,2,2,1,0,0},
    {0,0,1,5,2,2,1,1,1,1,2,2,5,1,0,0},
    {0,1,2,2,3,2,2,2,2,2,2,3,2,2,1,0},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,3,2,1,1,2,2,2,2,1,1,2,3,1,0},  // olhos 2px
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,2,2,2,4,4,4,4,4,4,2,2,2,1,0},
    {1,0,1,3,2,4,4,5,5,4,4,2,3,1,0,1},
    {0,0,1,2,2,4,1,4,1,4,2,2,1,0,0,0},
    {1,0,0,1,2,2,4,4,4,2,2,1,0,0,1,0},
    {0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0},
    {0,0,0,1,2,3,2,4,2,3,2,2,1,0,0,0},
    {0,0,1,2,2,2,3,4,3,2,2,2,2,1,0,0},
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},
    {0,0,1,2,2,2,2,4,2,2,2,2,2,1,0,0},
    {0,0,1,2,2,2,2,2,2,2,2,2,2,1,1,0},
    {0,0,1,1,2,1,1,2,1,1,2,1,1,2,2,1},
    {0,0,0,1,1,0,0,1,0,0,1,0,0,1,2,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
};

// Alert — olhos arregalados, boca aberta
const uint8_t CAT_ALERT[21][16] = {
    {0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0},  // orelhas mais altas
    {0,1,2,2,1,0,0,0,0,0,0,1,2,2,1,0},
    {0,1,5,2,2,1,1,1,1,1,1,2,2,5,1,0},
    {0,1,2,2,3,2,2,2,2,2,2,3,2,2,1,0},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,3,1,1,1,2,2,2,1,1,1,2,3,1,0},  // olhos 3px (arregalados)
    {0,1,2,2,4,1,2,2,2,4,1,2,2,2,1,0},  // brilho nos olhos
    {0,1,2,2,2,4,4,4,4,4,4,2,2,2,1,0},
    {1,0,1,3,2,4,4,5,5,4,4,2,3,1,0,1},
    {0,0,1,2,2,4,4,1,4,4,2,2,1,0,0,0},  // boca aberta
    {1,0,0,1,2,2,4,4,4,2,2,1,0,0,1,0},
    {0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0},
    {0,0,0,1,2,3,2,4,2,3,2,2,1,0,0,0},
    {0,0,1,2,2,2,3,4,3,2,2,2,2,1,0,0},
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},
    {0,0,1,2,2,2,2,4,2,2,2,2,2,1,0,0},
    {0,0,1,2,2,2,2,2,2,2,2,2,2,1,1,0},
    {0,0,1,1,2,1,1,2,1,1,2,1,1,2,2,1},
    {0,0,0,1,1,0,0,1,0,0,1,0,0,1,2,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
};

// ========== NETWORK ==========

bool networkIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool networkPoll(String& eventJson, String& serverTime) {
    if (!networkIsConnected()) return false;

    HTTPClient http;
    String url = String(API_BASE_URL) + "/device/next-event";
    http.begin(url);
    http.addHeader("X-Device-Token", DEVICE_TOKEN);
    http.setTimeout(10000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[http] poll failed, code=%d\n", code);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[json] parse error: %s\n", err.c_str());
        return false;
    }

    serverTime = doc["currentTime"].as<String>();

    if (doc["event"].is<JsonObject>()) {
        String buf;
        serializeJson(doc["event"], buf);
        eventJson = buf;
    } else {
        eventJson = "";
    }

    Serial.printf("[poll] ok, event=%s\n", eventJson.isEmpty() ? "none" : "yes");
    return true;
}

// ========== STATE ==========

time_t currentEpoch() {
    if (syncedEpoch == 0) return 0;
    unsigned long elapsed = (millis() - syncedMillis) / 1000;
    return syncedEpoch + (time_t)elapsed;
}

time_t parseISO(const String& iso) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d",
           &t.tm_year, &t.tm_mon, &t.tm_mday,
           &t.tm_hour, &t.tm_min, &t.tm_sec);
    t.tm_year -= 1900;
    t.tm_mon  -= 1;
    return mktime(&t) - _timezone;
}

void stateSyncTime(const String& isoTime) {
    syncedEpoch  = parseISO(isoTime);
    syncedMillis = millis();
    Serial.printf("[time] synced to epoch %ld\n", (long)syncedEpoch);
}

void stateSetEvent(const String& eventJson) {
    if (eventJson.isEmpty()) {
        currentEvent.valid = false;
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, eventJson)) {
        currentEvent.valid = false;
        return;
    }
    currentEvent.title              = doc["title"].as<String>();
    currentEvent.startsAt           = doc["startsAt"].as<String>();
    currentEvent.alertMinutesBefore = doc["alertMinutesBefore"] | 5;
    currentEvent.valid              = true;
}

String stateGetTimeStr() {
    time_t t = currentEpoch();
    if (t == 0) return "--:--";
    struct tm* info = gmtime(&t);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", info->tm_hour, info->tm_min);
    return String(buf);
}

String stateGetDateStr() {
    time_t t = currentEpoch();
    if (t == 0) return "--/--";
    struct tm* info = gmtime(&t);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d/%02d", info->tm_mday, info->tm_mon + 1);
    return String(buf);
}

int stateMinutesUntilEvent() {
    if (!currentEvent.valid) return 9999;
    time_t now   = currentEpoch();
    time_t event = parseISO(currentEvent.startsAt);
    return (int)((event - now) / 60);
}

void stateUpdate() {
    if (!networkIsConnected()) {
        if (currentState != STATE_CONNECTING) currentState = STATE_CONNECTING;
        return;
    }

    if (currentState == STATE_CONNECTING) {
        currentState = STATE_IDLE;
        Serial.println("[state] connected");
    }

    unsigned long now = millis();
    if (firstPoll || (now - lastPollMs >= POLL_INTERVAL_MS)) {
        firstPoll = false;
        lastPollMs = now;

        String eventJson, serverTime;
        if (networkPoll(eventJson, serverTime)) {
            stateSyncTime(serverTime);
            stateSetEvent(eventJson);
        } else {
            currentState = STATE_ERROR;
            return;
        }
    }

    if (currentEvent.valid) {
        int mins = stateMinutesUntilEvent();
        if (mins >= 0 && mins <= currentEvent.alertMinutesBefore) {
            currentState = STATE_ALERT;
        } else if (mins < 0) {
            currentEvent.valid = false;
            currentState = STATE_IDLE;
        } else {
            currentState = STATE_IDLE;
        }
    } else {
        if (currentState == STATE_ALERT) currentState = STATE_IDLE;
    }
}

// ========== RENDER ==========

void drawBitmap(const uint8_t bitmap[][16], int ox, int oy) {
    for (int row = 0; row < CAT_H; row++) {
        for (int col = 0; col < CAT_W; col++) {
            uint8_t idx = bitmap[row][col];
            if (idx != 0) {
                sprite.fillRect(ox + col * SCALE, oy + row * SCALE,
                                SCALE, SCALE, PALETTE[idx]);
            }
        }
    }
}

void renderFrame() {
    sprite.fillSprite(BG_COLOR);

    int ox = (W - CAT_W * SCALE) / 2;

    if (currentState == STATE_CONNECTING) {
        drawBitmap(CAT_IDLE, ox, 100);
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(TEXT_DIM);
        sprite.drawString("Conectando WiFi", CX, 40, 4);
        int dots = (millis() / 500) % 4;
        String d = "";
        for (int i = 0; i < dots; i++) d += ".";
        sprite.drawString(d, CX, 68, 4);
        sprite.pushSprite(0, 0);
        return;
    }

    if (currentState == STATE_ERROR) {
        drawBitmap(CAT_ALERT, ox, 100);
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(ALERT_COLOR);
        sprite.drawString("Erro de conexao", CX, 40, 4);
        sprite.setTextColor(TEXT_DIM);
        sprite.drawString("Tentando novamente...", CX, 68, 2);
        sprite.pushSprite(0, 0);
        return;
    }

    // --- Hora e data ---
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(TEXT_COLOR);
    sprite.drawString(stateGetTimeStr(), CX, 14, 6);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString(stateGetDateStr(), CX, 62, 4);

    // --- Gatinho ---
    int oy = 90;
    if (currentState == STATE_ALERT) {
        drawBitmap(CAT_ALERT, ox, oy);
    } else {
        drawBitmap(CAT_IDLE, ox, oy);
    }

    // --- Info do evento ---
    int bottomY = oy + CAT_H * SCALE + 10;

    if (currentState == STATE_ALERT) {
        int mins = stateMinutesUntilEvent();

        sprite.fillRoundRect(8, bottomY, W - 16, 70, 8, ALERT_BG);

        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(ALERT_COLOR);
        sprite.drawString("! EVENTO !", CX, bottomY + 8, 2);

        sprite.setTextColor(TEXT_COLOR);
        String title = currentEvent.title;
        if (title.length() > 18) title = title.substring(0, 16) + "..";
        sprite.drawString(title, CX, bottomY + 28, 4);

        sprite.setTextColor(TEXT_DIM);
        String countdown;
        if (mins <= 0)      countdown = "Agora!";
        else if (mins == 1) countdown = "em 1 min";
        else                countdown = "em " + String(mins) + " min";
        sprite.drawString(countdown, CX, bottomY + 52, 2);
    } else {
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(TEXT_DIM);
        sprite.drawString("Sem eventos proximos", CX, bottomY + 20, 2);
    }

    sprite.pushSprite(0, 0);
}

// ========== SETUP & LOOP ==========

void setup() {
    Serial.begin(115200);
    Serial.println("\n[gatinho-pet] booting...");

    // Display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BG_COLOR);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    sprite.createSprite(W, H);
    sprite.setSwapBytes(true);

    // State
    currentState = STATE_CONNECTING;
    setenv("TZ", "UTC0", 1);
    tzset();

    // WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[wifi] connecting");

    renderFrame();
}

void loop() {
    stateUpdate();

    unsigned long now = millis();
    if (now - lastRenderMs >= 1000) {
        lastRenderMs = now;
        renderFrame();
    }
}
