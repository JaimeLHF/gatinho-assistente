#include "render.h"
#include "state.h"
#include "sprites/frames.h"
#include "sprites/butterfly.h"

static TFT_eSPI tft;
static TFT_eSprite sprite(&tft);

// Display: 320 x 170 landscape
static const int W = 320;
static const int H = 170;
static const int CX = W / 2;

// ---- Colors (RGB565) ----
static const uint16_t COL_BG       = 0xCEFB;  // warm bege
static const uint16_t COL_GROUND   = 0x8C51;  // earthy olive
static const uint16_t COL_SHADOW   = 0xB5B6;  // subtle shadow
static const uint16_t COL_TEXT     = 0x3186;   // dark gray
static const uint16_t COL_TEXT_DIM = 0x7BCF;   // medium gray
static const uint16_t COL_ALERT_FG = 0xF800;   // red (error text)

// Cat position: centered horizontally, vertically centered +4px
static const int CAT_OX = (W - CAT_FRAME_W) / 2;     // 111
static const int CAT_OY = (H - CAT_FRAME_H) / 2 + 4; // ~35

// ---- Drawing helpers ----

static void drawCatFrame(int frameIdx, int ox, int oy) {
    const uint16_t* frame = cat_frames[frameIdx];
    for (int row = 0; row < CAT_FRAME_H; row++) {
        for (int col = 0; col < CAT_FRAME_W; col++) {
            uint16_t color = pgm_read_word(&frame[row * CAT_FRAME_W + col]);
            if (color != CAT_TRANSPARENT) {
                sprite.drawPixel(ox + col, oy + row, color);
            }
        }
    }
}

static void drawScene() {
    // Ground strip at bottom
    sprite.fillRect(0, H - 8, W, 8, COL_GROUND);

    // Shadow ellipse under cat
    int sx = CAT_OX + CAT_FRAME_W / 2;
    int sy = CAT_OY + CAT_FRAME_H + 4;
    sprite.fillEllipse(sx, sy, 28, 4, COL_SHADOW);
}

// ---- State screens ----

static void drawConnecting() {
    sprite.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_FRAME_IDLE, CAT_OX, CAT_OY);

    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(COL_TEXT_DIM, COL_BG);
    sprite.drawString("Conectando WiFi", CX, 8, 2);

    int dots = (millis() / 500) % 4;
    String d = "";
    for (int i = 0; i < dots; i++) d += ".";
    sprite.drawString(d, CX, 24, 2);
}

static void drawError() {
    sprite.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_FRAME_ALERT, CAT_OX, CAT_OY);

    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(COL_ALERT_FG, COL_BG);
    sprite.drawString("Erro de conexao", CX, 8, 2);
    sprite.setTextColor(COL_TEXT_DIM, COL_BG);
    sprite.drawString("Tentando novamente...", CX, 24, 2);
}

// ---- Public API ----

void renderSetup() {
    tft.init();
    tft.setRotation(1);  // landscape 320x170
    tft.fillScreen(COL_BG);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    sprite.createSprite(W, H);
    sprite.setSwapBytes(true);
}

void renderFrame(AppState state) {
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

    // IDLE or ALERT — idle cat for now (alert frame in commit 5)
    sprite.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_FRAME_IDLE, CAT_OX, CAT_OY);

    sprite.pushSprite(0, 0);
}
