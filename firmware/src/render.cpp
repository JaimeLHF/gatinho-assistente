#include "render.h"
#include "state.h"
#include "sprites/frames.h"
#include "sprites/butterfly.h"
#include <math.h>

static TFT_eSPI tft;
static TFT_eSprite fb(&tft);

// Display: 320 x 170 landscape
static const int W = 320;
static const int H = 170;
static const int CX = W / 2;
static const int SCALE = 1;

// ---- Colors (RGB565 native — sem swap, sem inversao) ----
static const uint16_t COL_BG        = 0x3186;  // cinza chumbo escuro
static const uint16_t COL_GROUND    = 0x6BC4;  // earthy olive
static const uint16_t COL_GROUND_HI = 0x8C68;  // ground highlight
static const uint16_t COL_SHADOW    = 0x18E3;  // sombra mais sutil
static const uint16_t COL_TEXT      = 0xE73C;  // off-white pra contraste
static const uint16_t COL_TEXT_DIM  = 0x7BCF;  // medium gray
static const uint16_t COL_ALERT_FG  = 0xF800;  // red
static const uint16_t COL_HEART     = 0xFB14;  // heart pink
static const uint16_t COL_HEART_HI  = 0xFD9F;  // heart highlight

// Cat position: centered horizontally, vertically centered +4px
static const int CAT_OX = (W - CAT_FRAME_W * SCALE) / 2;
static const int CAT_OY = (H - CAT_FRAME_H * SCALE) / 2 + 4;

// ---- Buttons ----
static const int BTN_BOOT = 0;   // GPIO 0 (BOOT)
static const int BTN_KEY  = 14;  // GPIO 14 (KEY)

// ---- Animation state ----
static unsigned long lastInteraction = 0;
static const unsigned long SLEEP_TIMEOUT_MS = 30000;  // 30s

// Blink
static unsigned long nextBlinkMs = 0;
static unsigned long blinkEndMs  = 0;
static const unsigned long BLINK_DURATION_MS  = 500;
static const unsigned long BLINK_INTERVAL_MIN = 3000;
static const unsigned long BLINK_INTERVAL_MAX = 7000;

// Yawn (rising edge: sleep → awake)
static bool wasSleeping    = false;
static unsigned long yawnEndMs = 0;
static const unsigned long YAWN_DURATION_MS = 1500;

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
    // Shadow ellipse under cat (acompanha breathing)
    int sx = CAT_OX + (CAT_FRAME_W * SCALE) / 2;
    int sy = CAT_OY + 96;  // fixa no chão, gato respira por cima
    fb.fillEllipse(sx, sy, 24, 3, COL_SHADOW);
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

// ---- Button reading ----
static void updateButtons() {
    if (digitalRead(BTN_BOOT) == LOW || digitalRead(BTN_KEY) == LOW) {
        lastInteraction = millis();
    }
}

// ---- Determine which cat frame to show ----
static int chooseCatFrame(unsigned long now, AppState appState) {
    // ALERT from API has highest priority
    if (appState == STATE_ALERT) return CAT_FRAME_ALERT;

    bool sleeping = (now - lastInteraction) >= SLEEP_TIMEOUT_MS;

    // Detect rising edge: was sleeping, now awake → yawn
    if (wasSleeping && !sleeping) {
        yawnEndMs = now + YAWN_DURATION_MS;
    }
    wasSleeping = sleeping;

    // Yawning (just woke up)
    if (!sleeping && now < yawnEndMs) return CAT_FRAME_YAWN;

    // Sleeping
    if (sleeping) return CAT_FRAME_SLEEP;

    // Blink timer
    if (now < blinkEndMs) return CAT_FRAME_BLINK;
    if (now >= nextBlinkMs) {
        blinkEndMs = now + BLINK_DURATION_MS;
        nextBlinkMs = now + BLINK_INTERVAL_MIN + (random(BLINK_INTERVAL_MAX - BLINK_INTERVAL_MIN));
    }

    return CAT_FRAME_IDLE;
}

void renderSetup() {
    // CRÍTICO: alimentar o painel LCD via GPIO 15
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    // Backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    // Buttons
    pinMode(BTN_BOOT, INPUT_PULLUP);
    pinMode(BTN_KEY, INPUT_PULLUP);

    tft.init();
    tft.setRotation(1);  // landscape 320x170
    tft.fillScreen(COL_BG);

    fb.setColorDepth(16);
    fb.createSprite(W, H);

    // Init animation timers
    lastInteraction = millis();
    nextBlinkMs = millis() + 3000 + random(4000);
}

void renderFrame(AppState state) {
    updateButtons();

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
    unsigned long now = millis();
    int frame = chooseCatFrame(now, state);

    // Breathing bob when sleeping (sin wave, ~1-2px vertical)
    int bob = 0;
    if (frame == CAT_FRAME_SLEEP) {
        bob = (int)(sinf(now * 0.0018f) * 2.0f);
    }

    fb.fillSprite(COL_BG);
    if (frame != CAT_FRAME_SLEEP) drawScene();
    drawCatFrame(CAT_OX, CAT_OY + bob, frame);
    fb.pushSprite(0, 0);
}
