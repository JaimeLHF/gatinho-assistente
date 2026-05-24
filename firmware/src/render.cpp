#include "render.h"
#include "state.h"
#include "sprites/frames.h"
#include "sprites/butterfly.h"

static TFT_eSPI tft;
static TFT_eSprite sprite(&tft);

// Display: 170 x 320 portrait
static const int W = 170;
static const int H = 320;
static const int CX = W / 2;

// ---- Color Palette (RGB565) ----
static const uint16_t BG_COLOR    = 0x1926;  // dark navy
static const uint16_t COL_BLACK   = 0x0000;
static const uint16_t COL_BODY    = 0xE504;  // golden orange
static const uint16_t COL_STRIPE  = 0xB380;  // dark orange/brown
static const uint16_t COL_CREAM   = 0xF71B;  // light cream
static const uint16_t COL_PINK    = 0xEB2D;  // pink
static const uint16_t TEXT_COLOR  = 0xFFFF;
static const uint16_t TEXT_DIM    = 0x9CF3;  // light gray
static const uint16_t ALERT_COLOR = 0xFD20;  // bright yellow
static const uint16_t ALERT_BG    = 0x6180;  // dark warm

// Palette: 0=transparent, 1=black, 2=body, 3=stripe, 4=cream, 5=pink
static const uint16_t PALETTE[] = {
    BG_COLOR, COL_BLACK, COL_BODY, COL_STRIPE, COL_CREAM, COL_PINK
};

// ---- Pixel Art Bitmaps: 16 x 21 ----
static const uint8_t CAT_W = 16;
static const uint8_t CAT_H = 21;
static const uint8_t SCALE = 5;

// Idle cat — relaxed eyes, normal ears
static const uint8_t CAT_IDLE[21][16] = {
    {0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0},  //  0  ear tips
    {0,0,1,2,2,1,0,0,0,0,1,2,2,1,0,0},  //  1  ears
    {0,0,1,5,2,2,1,1,1,1,2,2,5,1,0,0},  //  2  inner ears + head top
    {0,1,2,2,3,2,2,2,2,2,2,3,2,2,1,0},  //  3  head + stripes
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  4  head
    {0,1,3,2,1,1,2,2,2,2,1,1,2,3,1,0},  //  5  eyes (relaxed, 2px)
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  6  below eyes
    {0,1,2,2,2,4,4,4,4,4,4,2,2,2,1,0},  //  7  muzzle
    {1,0,1,3,2,4,4,5,5,4,4,2,3,1,0,1},  //  8  nose + whisker dots
    {0,0,1,2,2,4,1,4,1,4,2,2,1,0,0,0},  //  9  mouth
    {1,0,0,1,2,2,4,4,4,2,2,1,0,0,1,0},  // 10  chin + whisker dots
    {0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0},  // 11  neck
    {0,0,0,1,2,3,2,4,2,3,2,2,1,0,0,0},  // 12  chest
    {0,0,1,2,2,2,3,4,3,2,2,2,2,1,0,0},  // 13  body
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 14  arms
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 15  arms
    {0,0,1,2,2,2,2,4,2,2,2,2,2,1,0,0},  // 16  lower body
    {0,0,1,2,2,2,2,2,2,2,2,2,2,1,1,0},  // 17  above feet
    {0,0,1,1,2,1,1,2,1,1,2,1,1,2,2,1},  // 18  feet + tail
    {0,0,0,1,1,0,0,1,0,0,1,0,0,1,2,1},  // 19  soles + tail
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},  // 20  tail tip
};

// Alert cat — wide eyes, open mouth
static const uint8_t CAT_ALERT[21][16] = {
    {0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0},  //  0  taller ear tips
    {0,1,2,2,1,0,0,0,0,0,0,1,2,2,1,0},  //  1  ears wider
    {0,1,5,2,2,1,1,1,1,1,1,2,2,5,1,0},  //  2  inner ears + head top
    {0,1,2,2,3,2,2,2,2,2,2,3,2,2,1,0},  //  3  head + stripes
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  4  head
    {0,1,3,1,1,1,2,2,2,1,1,1,2,3,1,0},  //  5  eyes WIDE (3px each)
    {0,1,2,2,4,1,2,2,2,4,1,2,2,2,1,0},  //  6  eye highlights
    {0,1,2,2,2,4,4,4,4,4,4,2,2,2,1,0},  //  7  muzzle
    {1,0,1,3,2,4,4,5,5,4,4,2,3,1,0,1},  //  8  nose + whisker dots
    {0,0,1,2,2,4,4,1,4,4,2,2,1,0,0,0},  //  9  open mouth
    {1,0,0,1,2,2,4,4,4,2,2,1,0,0,1,0},  // 10  chin + whisker dots
    {0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0},  // 11  neck
    {0,0,0,1,2,3,2,4,2,3,2,2,1,0,0,0},  // 12  chest
    {0,0,1,2,2,2,3,4,3,2,2,2,2,1,0,0},  // 13  body
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 14  arms
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 15  arms
    {0,0,1,2,2,2,2,4,2,2,2,2,2,1,0,0},  // 16  lower body
    {0,0,1,2,2,2,2,2,2,2,2,2,2,1,1,0},  // 17  above feet
    {0,0,1,1,2,1,1,2,1,1,2,1,1,2,2,1},  // 18  feet + tail
    {0,0,0,1,1,0,0,1,0,0,1,0,0,1,2,1},  // 19  soles + tail
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},  // 20  tail tip
};

// ---- Drawing helpers ----

static void drawBitmap(const uint8_t bitmap[][16], int ox, int oy) {
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

static void drawConnecting() {
    sprite.fillSprite(BG_COLOR);

    // Cat idle centered
    int ox = (W - CAT_W * SCALE) / 2;
    drawBitmap(CAT_IDLE, ox, 100);

    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString("Conectando WiFi", CX, 40, 4);

    int dots = (millis() / 500) % 4;
    String d = "";
    for (int i = 0; i < dots; i++) d += ".";
    sprite.drawString(d, CX, 68, 4);
}

static void drawError() {
    sprite.fillSprite(BG_COLOR);

    int ox = (W - CAT_W * SCALE) / 2;
    drawBitmap(CAT_ALERT, ox, 100);

    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(ALERT_COLOR);
    sprite.drawString("Erro de conexao", CX, 40, 4);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString("Tentando novamente...", CX, 68, 2);
}

void renderSetup() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BG_COLOR);

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
    sprite.drawString(stateGetTimeStr(), CX, 14, 6);
    sprite.setTextColor(TEXT_DIM);
    sprite.drawString(stateGetDateStr(), CX, 62, 4);

    // --- Center: cat ---
    int catH = CAT_H * SCALE;  // 105
    int ox = (W - CAT_W * SCALE) / 2;
    int oy = 90;

    if (state == STATE_ALERT) {
        drawBitmap(CAT_ALERT, ox, oy);
    } else {
        drawBitmap(CAT_IDLE, ox, oy);
    }

    // --- Bottom: event info ---
    int bottomY = oy + catH + 10;

    if (state == STATE_ALERT) {
        EventData ev = stateGetEvent();
        int mins = stateMinutesUntilEvent();

        sprite.fillRoundRect(8, bottomY, W - 16, 70, 8, ALERT_BG);

        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(ALERT_COLOR);
        sprite.drawString("! EVENTO !", CX, bottomY + 8, 2);

        sprite.setTextColor(TEXT_COLOR);
        String title = ev.title;
        if (title.length() > 18) title = title.substring(0, 16) + "..";
        sprite.drawString(title, CX, bottomY + 28, 4);

        sprite.setTextColor(TEXT_DIM);
        String countdown;
        if (mins <= 0) {
            countdown = "Agora!";
        } else if (mins == 1) {
            countdown = "em 1 min";
        } else {
            countdown = "em " + String(mins) + " min";
        }
        sprite.drawString(countdown, CX, bottomY + 52, 2);
    } else {
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(TEXT_DIM);
        sprite.drawString("Sem eventos proximos", CX, bottomY + 20, 2);
    }

    sprite.pushSprite(0, 0);
}
