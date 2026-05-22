#include "render.h"
#include "state.h"

static TFT_eSPI tft;
static TFT_eSprite sprite(&tft);

// Colors (RGB565)
static const uint16_t BG_COLOR      = 0x1082; // dark gray-blue
static const uint16_t CAT_COLOR     = 0xFBE0; // warm orange
static const uint16_t CAT_DARK      = 0xC360; // darker orange
static const uint16_t EYE_WHITE     = 0xFFFF;
static const uint16_t EYE_PUPIL     = 0x18C3; // dark
static const uint16_t NOSE_COLOR    = 0xFACB; // pinkish
static const uint16_t TEXT_COLOR    = 0xFFFF;
static const uint16_t TEXT_DIM      = 0x9CF3; // light gray
static const uint16_t ALERT_COLOR   = 0xFD20; // bright yellow
static const uint16_t ALERT_BG      = 0x6180; // dark warm

// Display: 170 x 320 portrait
static const int W = 170;
static const int H = 320;

// Cat center position
static const int CX = W / 2;
static const int CY = 150;

static void drawCatIdle() {
    // Head
    sprite.fillCircle(CX, CY, 38, CAT_COLOR);

    // Ears (triangles)
    sprite.fillTriangle(CX - 35, CY - 28, CX - 22, CY - 55, CX - 8, CY - 30, CAT_COLOR);
    sprite.fillTriangle(CX + 35, CY - 28, CX + 22, CY - 55, CX + 8, CY - 30, CAT_COLOR);
    // Inner ears
    sprite.fillTriangle(CX - 30, CY - 30, CX - 22, CY - 48, CX - 13, CY - 31, NOSE_COLOR);
    sprite.fillTriangle(CX + 30, CY - 30, CX + 22, CY - 48, CX + 13, CY - 31, NOSE_COLOR);

    // Eyes (relaxed, half-closed)
    sprite.fillEllipse(CX - 14, CY - 4, 7, 5, EYE_WHITE);
    sprite.fillEllipse(CX + 14, CY - 4, 7, 5, EYE_WHITE);
    sprite.fillEllipse(CX - 14, CY - 3, 4, 4, EYE_PUPIL);
    sprite.fillEllipse(CX + 14, CY - 3, 4, 4, EYE_PUPIL);

    // Nose
    sprite.fillTriangle(CX - 3, CY + 8, CX + 3, CY + 8, CX, CY + 12, NOSE_COLOR);

    // Mouth
    sprite.drawLine(CX, CY + 12, CX - 6, CY + 17, CAT_DARK);
    sprite.drawLine(CX, CY + 12, CX + 6, CY + 17, CAT_DARK);

    // Whiskers
    sprite.drawLine(CX - 18, CY + 6, CX - 45, CY + 0, TEXT_DIM);
    sprite.drawLine(CX - 18, CY + 10, CX - 44, CY + 10, TEXT_DIM);
    sprite.drawLine(CX + 18, CY + 6, CX + 45, CY + 0, TEXT_DIM);
    sprite.drawLine(CX + 18, CY + 10, CX + 44, CY + 10, TEXT_DIM);
}

static void drawCatAlert() {
    // Head
    sprite.fillCircle(CX, CY, 38, CAT_COLOR);

    // Ears (perked up, taller)
    sprite.fillTriangle(CX - 35, CY - 28, CX - 18, CY - 62, CX - 5, CY - 30, CAT_COLOR);
    sprite.fillTriangle(CX + 35, CY - 28, CX + 18, CY - 62, CX + 5, CY - 30, CAT_COLOR);
    sprite.fillTriangle(CX - 30, CY - 30, CX - 18, CY - 54, CX - 10, CY - 31, NOSE_COLOR);
    sprite.fillTriangle(CX + 30, CY - 30, CX + 18, CY - 54, CX + 10, CY - 31, NOSE_COLOR);

    // Eyes (wide open, surprised)
    sprite.fillCircle(CX - 14, CY - 4, 9, EYE_WHITE);
    sprite.fillCircle(CX + 14, CY - 4, 9, EYE_WHITE);
    sprite.fillCircle(CX - 14, CY - 4, 5, EYE_PUPIL);
    sprite.fillCircle(CX + 14, CY - 4, 5, EYE_PUPIL);
    // Highlight
    sprite.fillCircle(CX - 12, CY - 6, 2, EYE_WHITE);
    sprite.fillCircle(CX + 12, CY - 6, 2, EYE_WHITE);

    // Nose
    sprite.fillTriangle(CX - 3, CY + 8, CX + 3, CY + 8, CX, CY + 12, NOSE_COLOR);

    // Open mouth (surprised)
    sprite.fillEllipse(CX, CY + 18, 5, 4, CAT_DARK);

    // Whiskers (raised)
    sprite.drawLine(CX - 18, CY + 4, CX - 46, CY - 6, TEXT_COLOR);
    sprite.drawLine(CX - 18, CY + 8, CX - 46, CY + 4, TEXT_COLOR);
    sprite.drawLine(CX + 18, CY + 4, CX + 46, CY - 6, TEXT_COLOR);
    sprite.drawLine(CX + 18, CY + 8, CX + 46, CY + 4, TEXT_COLOR);
}

static void drawConnecting() {
    sprite.fillSprite(BG_COLOR);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString("Conectando WiFi...", CX, CY, 4);

    // Animated dots based on millis
    int dots = (millis() / 500) % 4;
    String d = "";
    for (int i = 0; i < dots; i++) d += ".";
    sprite.drawString(d, CX, CY + 30, 4);
}

static void drawError() {
    sprite.fillSprite(BG_COLOR);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(ALERT_COLOR);
    sprite.drawString("Erro de conexao", CX, CY - 10, 4);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString("Tentando novamente...", CX, CY + 20, 2);
}

void renderSetup() {
    tft.init();
    tft.setRotation(0); // portrait
    tft.fillScreen(BG_COLOR);

    // Backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    sprite.createSprite(W, H);
    sprite.setSwapBytes(true);
}

void renderFrame(AppState state) {
    sprite.fillSprite(BG_COLOR);

    if (state == STATE_CONNECTING) {
        drawConnecting();
        sprite.pushSprite(0, 0);
        return;
    }

    if (state == STATE_ERROR) {
        drawError();
        sprite.pushSprite(0, 0);
        return;
    }

    // --- Top: time and date ---
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(TEXT_COLOR);
    sprite.drawString(stateGetTimeStr(), CX, 18, 6);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString(stateGetDateStr(), CX, 68, 4);

    // --- Center: cat ---
    if (state == STATE_ALERT) {
        drawCatAlert();
    } else {
        drawCatIdle();
    }

    // --- Bottom: event info ---
    if (state == STATE_ALERT) {
        EventData ev = stateGetEvent();
        int mins = stateMinutesUntilEvent();

        // Alert background bar
        sprite.fillRoundRect(8, 210, W - 16, 70, 8, ALERT_BG);

        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(ALERT_COLOR);
        sprite.drawString("! EVENTO !", CX, 218, 2);

        sprite.setTextColor(TEXT_COLOR);
        // Truncate title if too long
        String title = ev.title;
        if (title.length() > 18) title = title.substring(0, 16) + "..";
        sprite.drawString(title, CX, 238, 4);

        sprite.setTextColor(TEXT_DIM);
        String countdown;
        if (mins <= 0) {
            countdown = "Agora!";
        } else if (mins == 1) {
            countdown = "em 1 min";
        } else {
            countdown = "em " + String(mins) + " min";
        }
        sprite.drawString(countdown, CX, 262, 2);
    } else {
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(TEXT_DIM);
        sprite.drawString("Sem eventos proximos", CX, 240, 2);
    }

    sprite.pushSprite(0, 0);
}
