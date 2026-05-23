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
 * Frames (via Piskel → png_to_h.py → frames.h):
 *   0 = idle (olhos abertos)
 *   1 = blink (olhos fechados)
 *   2 = wave A (acenando pos 1)
 *   3 = wave B (acenando pos 2)
 *
 * Estados:
 *   CONNECTING → frame 0 + "Conectando WiFi..."
 *   IDLE       → frame 0, pisca a cada 5s (frame 1)
 *   ALERT      → cicla frames 2-3 (acenando) + barra de evento
 *   ERROR      → frame 1 + "Erro de conexao"
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "frames.h"

// =============================================================
// >>>  CONFIGURE AQUI  <<<
// =============================================================
#define WIFI_SSID      "SEU_WIFI"
#define WIFI_PASSWORD  "SUA_SENHA"
#define API_BASE_URL   "http://192.168.1.100:3001/api/v1"
#define DEVICE_TOKEN   "SEU_TOKEN_DE_64_CARACTERES_HEX"
#define POLL_INTERVAL_MS 60000
// =============================================================

// ---- Hardware ----
#define LCD_POWER_PIN  15
#define LCD_BL_PIN     38

// ---- Display (portrait) ----
TFT_eSPI    tft;
TFT_eSprite fb(&tft);
#define SCREEN_W 170
#define SCREEN_H 320
#define CX (SCREEN_W / 2)

// ---- Cores cenario ----
#define COL_BG        0x1926  // fundo azul escuro
#define COL_GROUND    0x6BC4
#define COL_GROUND_HI 0x8C68
#define COL_SHADOW    0x528A
#define TEXT_WHITE    0xFFFF
#define TEXT_DIM      0x9CF3
#define ALERT_COLOR   0xFD20  // amarelo
#define ALERT_BG      0x6180

// ---- Cat positioning ----
#define CAT_SCALE  1   // arte ja esta no tamanho certo (98x107)
#define CAT_OX     ((SCREEN_W - CAT_FRAME_W) / 2)
#define CAT_OY     88

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

// ---- Time sync ----
static time_t      syncedEpoch  = 0;
static unsigned long syncedMillis = 0;

// ---- Animation ----
static unsigned long lastRenderMs  = 0;
static bool          blinking      = false;
static unsigned long blinkStart    = 0;
static unsigned long nextBlinkAt   = 3000;
static int           waveIdx       = 0;
static unsigned long lastWaveFrame = 0;

#define BLINK_INTERVAL  5000
#define BLINK_DUR_MS    200
#define WAVE_FRAME_MS   300
#define RENDER_INTERVAL 33   // ~30 fps

// ========== RENDER ==========

static void drawCatFrame(int ox, int oy, int frameIdx) {
    for (int y = 0; y < CAT_FRAME_H; y++) {
        for (int x = 0; x < CAT_FRAME_W; x++) {
            uint16_t p = pgm_read_word(&cat_frames[frameIdx][y * CAT_FRAME_W + x]);
            if (p == CAT_TRANSPARENT) continue;
            fb.drawPixel(ox + x, oy + y, p);
        }
    }
}

static void drawGround() {
    int groundY = CAT_OY + CAT_FRAME_H + 4;
    fb.fillRect(0, groundY, SCREEN_W, SCREEN_H - groundY, COL_GROUND);
    fb.drawFastHLine(0, groundY - 1, SCREEN_W, 0x4082);
    for (int x = 0; x < SCREEN_W; x += 6) {
        fb.fillRect(x, groundY + 1, 1, 2, COL_GROUND_HI);
    }
    // sombra do gato
    int shadowMargin = CAT_FRAME_W / 8;
    fb.fillRect(CAT_OX + shadowMargin, groundY - 2,
                CAT_FRAME_W - 2 * shadowMargin, 2, COL_SHADOW);
}

static void drawTimeBar() {
    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(TEXT_WHITE);
    fb.drawString(stateGetTimeStr(), CX, 10, 6);
    fb.setTextColor(TEXT_DIM);
    fb.drawString(stateGetDateStr(), CX, 58, 4);
}

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

// ========== RENDER FRAMES ==========

void renderFrame() {
    unsigned long now = millis();
    fb.fillSprite(COL_BG);

    // ---- Determine animation frame ----
    int frameIdx = 0;

    switch (currentState) {
        case STATE_CONNECTING:
            frameIdx = 0;
            // Texto
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(TEXT_DIM);
            fb.drawString("Conectando WiFi", CX, 20, 4);
            {
                int dots = (now / 500) % 4;
                String d = "";
                for (int i = 0; i < dots; i++) d += ".";
                fb.drawString(d, CX, 48, 4);
            }
            break;

        case STATE_ERROR:
            frameIdx = 1;  // olhos fechados
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(ALERT_COLOR);
            fb.drawString("Erro de conexao", CX, 10, 4);
            fb.setTextColor(TEXT_DIM);
            fb.drawString("Tentando novamente...", CX, 40, 2);
            break;

        case STATE_ALERT:
            // Acenando — cicla frames 2-3
            if (now - lastWaveFrame >= WAVE_FRAME_MS) {
                waveIdx = (waveIdx + 1) % 2;
                lastWaveFrame = now;
            }
            frameIdx = 2 + waveIdx;
            drawTimeBar();
            break;

        case STATE_IDLE:
        default:
            // Piscar
            if (!blinking && now >= nextBlinkAt) { blinking = true; blinkStart = now; }
            if (blinking && now - blinkStart >= BLINK_DUR_MS) {
                blinking = false;
                nextBlinkAt = now + BLINK_INTERVAL;
            }
            frameIdx = blinking ? 1 : 0;
            drawTimeBar();
            break;
    }

    // ---- Chao + sombra ----
    drawGround();

    // ---- Gatinho ----
    drawCatFrame(CAT_OX, CAT_OY, frameIdx);

    // ---- Info do evento (abaixo do chao) ----
    int infoY = CAT_OY + CAT_FRAME_H + 10;

    if (currentState == STATE_ALERT) {
        int mins = stateMinutesUntilEvent();

        fb.fillRoundRect(6, infoY, SCREEN_W - 12, 60, 6, ALERT_BG);

        fb.setTextDatum(TC_DATUM);
        fb.setTextColor(ALERT_COLOR);
        fb.drawString("! EVENTO !", CX, infoY + 6, 2);

        fb.setTextColor(TEXT_WHITE);
        String title = currentEvent.title;
        if (title.length() > 18) title = title.substring(0, 16) + "..";
        fb.drawString(title, CX, infoY + 22, 4);

        fb.setTextColor(TEXT_DIM);
        String countdown;
        if (mins <= 0)      countdown = "Agora!";
        else if (mins == 1) countdown = "em 1 min";
        else                countdown = "em " + String(mins) + " min";
        fb.drawString(countdown, CX, infoY + 44, 2);
    } else if (currentState == STATE_IDLE) {
        fb.setTextDatum(TC_DATUM);
        fb.setTextColor(TEXT_DIM);
        fb.drawString("Sem eventos proximos", CX, infoY + 15, 2);
    }

    fb.pushSprite(0, 0);
}

// ========== SETUP & LOOP ==========

void setup() {
    Serial.begin(115200);
    Serial.println("\n[gatinho-pet] booting...");

    // Display
    pinMode(LCD_POWER_PIN, OUTPUT); digitalWrite(LCD_POWER_PIN, HIGH);
    pinMode(LCD_BL_PIN,    OUTPUT); digitalWrite(LCD_BL_PIN,    HIGH);

    tft.init();
    tft.setRotation(0);  // portrait 170x320
    tft.fillScreen(COL_BG);

    fb.setColorDepth(16);
    fb.createSprite(SCREEN_W, SCREEN_H);

    // Time
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
    if (now - lastRenderMs >= RENDER_INTERVAL) {
        lastRenderMs = now;
        renderFrame();
    }
}
