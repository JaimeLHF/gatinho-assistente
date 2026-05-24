#include "render.h"
#include "state.h"
#include "sprites/frames.h"
#include "sprites/butterfly.h"

static TFT_eSPI tft;
static TFT_eSprite fb(&tft);

// Display: 320 x 170 landscape
static const int W = 320;
static const int H = 170;
static const int CX = W / 2;
static const int SCALE = 1;

// ---- Colors (RGB565 native — sem swap, sem inversao) ----
static const uint16_t COL_BG        = 0xCEFB;  // warm bege
static const uint16_t COL_GROUND    = 0x6BC4;  // earthy olive
static const uint16_t COL_GROUND_HI = 0x8C68;  // ground highlight
static const uint16_t COL_SHADOW    = 0x528A;  // subtle shadow
static const uint16_t COL_TEXT      = 0x3186;  // dark gray
static const uint16_t COL_TEXT_DIM  = 0x7BCF;  // medium gray
static const uint16_t COL_ALERT_FG  = 0xF800;  // red
static const uint16_t COL_HEART     = 0xFB14;  // heart pink
static const uint16_t COL_HEART_HI  = 0xFD9F;  // heart highlight

// Cat position: centered horizontally, vertically centered +4px
static const int CAT_OX = (W - CAT_FRAME_W * SCALE) / 2;
static const int CAT_OY = (H - CAT_FRAME_H * SCALE) / 2 + 4;

// ---- Drawing helpers (pixel-a-pixel, sem pushImage, sem setSwapBytes) ----

static void drawCatFrame(int ox, int oy, int frameIdx) {
    for (int y = 0; y < CAT_FRAME_H; y++) {
        for (int x = 0; x < CAT_FRAME_W; x++) {
            uint16_t p = pgm_read_word(&cat_frames[frameIdx][y * CAT_FRAME_W + x]);
            if (p == CAT_TRANSPARENT) continue;
            if (SCALE == 1) {
                fb.drawPixel(ox + x, oy + y, p);
            } else {
                fb.fillRect(ox + x * SCALE, oy + y * SCALE, SCALE, SCALE, p);
            }
        }
    }
}

static void drawButterflyFrame(int ox, int oy, int frameIdx) {
    for (int y = 0; y < BTF_FRAME_H; y++) {
        for (int x = 0; x < BTF_FRAME_W; x++) {
            uint16_t p = pgm_read_word(&btf_frames[frameIdx][y * BTF_FRAME_W + x]);
            if (p == BTF_TRANSPARENT) continue;
            if (SCALE == 1) {
                fb.drawPixel(ox + x, oy + y, p);
            } else {
                fb.fillRect(ox + x * SCALE, oy + y * SCALE, SCALE, SCALE, p);
            }
        }
    }
}

static void drawScene() {
    // Ground strip at bottom
    fb.fillRect(0, H - 8, W, 8, COL_GROUND);
    // Ground highlight line
    fb.drawFastHLine(0, H - 8, W, COL_GROUND_HI);
    // Shadow ellipse under cat
    int sx = CAT_OX + (CAT_FRAME_W * SCALE) / 2;
    int sy = CAT_OY + CAT_FRAME_H * SCALE + 4;
    fb.fillEllipse(sx, sy, 28, 4, COL_SHADOW);
}

// ---- State screens ----

static void drawConnecting() {
    fb.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_IDLE);

    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Conectando WiFi", CX, 8, 2);

    int dots = (millis() / 500) % 4;
    String d = "";
    for (int i = 0; i < dots; i++) d += ".";
    fb.drawString(d, CX, 24, 2);
}

static void drawError() {
    fb.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_ALERT);

    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(COL_ALERT_FG, COL_BG);
    fb.drawString("Erro de conexao", CX, 8, 2);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Tentando novamente...", CX, 24, 2);
}

// ---- Public API ----

void renderSetup() {
    // CRÍTICO: alimentar o painel LCD via GPIO 15
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    // Backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    tft.init();
    tft.setRotation(1);  // landscape 320x170
    tft.fillScreen(COL_BG);

    fb.setColorDepth(16);
    fb.createSprite(W, H);
}

void renderFrame(AppState state) {
    if (state == STATE_CONNECTING) {
        drawConnecting();
        fb.pushSprite(0, 0);
        return;
    }

    if (state == STATE_ERROR) {
        drawError();
        fb.pushSprite(0, 0);
        return;
    }

    // IDLE or ALERT
    fb.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_IDLE);
    fb.pushSprite(0, 0);
}
