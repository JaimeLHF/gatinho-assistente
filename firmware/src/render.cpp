#include "render.h"
#include "state.h"
#include "time_sync.h"
#include "weather.h"
#include "wifi_portal.h"
#include "button_reset.h"
#include "sprites/frames.h"
#include "sprites/butterfly.h"
#include "sprites/error.h"
#include <WiFi.h>
#include <math.h>

static TFT_eSPI tft;
static TFT_eSprite fb(&tft);

// Display: 320 x 170 landscape
static const int W = 320;
static const int H = 170;
static const int CX = W / 2;

// ---- Colors (RGB565 native — sem swap, sem inversao) ----
static const uint16_t COL_BG        = 0x3186;  // cinza chumbo escuro
static const uint16_t COL_GROUND    = 0x6BC4;  // earthy olive
static const uint16_t COL_GROUND_HI = 0x8C68;  // ground highlight
static const uint16_t COL_SHADOW    = 0x18E3;  // sombra mais sutil
static const uint16_t COL_TEXT      = 0xE73C;  // off-white pra contraste
static const uint16_t COL_TEXT_DIM  = 0x7BCF;  // medium gray
static const uint16_t COL_ALERT_FG  = 0xFCC8;  // laranja suave
static const uint16_t COL_HEART     = 0xFB14;  // heart pink
static const uint16_t COL_HEART_HI  = 0xFD9F;  // heart highlight
static const uint16_t COL_SUN       = 0xFEE0;  // bright yellow
static const uint16_t COL_CLOUD     = 0xC618;  // light gray
static const uint16_t COL_RAIN      = 0x5D1F;  // blue
static const uint16_t COL_SNOW_DOT  = 0xFFFF;  // white

// Cat scaled up ~112%
static const int CAT_DRAW_W = 110;  // 98 * 1.12
static const int CAT_DRAW_H = 120;  // 107 * 1.12

// Cat position: left side, vertically centered
static const int CAT_OX = 16;
static const int CAT_OY = (H - CAT_DRAW_H) / 2 + 4;

// Text area: centered in the space right of the cat
static const int TEXT_AREA_X  = CAT_OX + CAT_DRAW_W + 4;
static const int TEXT_AREA_W  = W - TEXT_AREA_X - 4;
static const int TEXT_AREA_CX = TEXT_AREA_X + TEXT_AREA_W / 2;

// ---- Buttons ----
static const int BTN_BOOT = 0;   // GPIO 0 (BOOT)
static const int BTN_KEY  = 14;  // GPIO 14 (KEY)

// ---- Animation state ----
static unsigned long lastInteraction = 0;

// Clock digit slide animation
static char prevClock[6] = {'-','-',':','-','-','\0'};
static unsigned long clockAnimMs[5] = {0,0,0,0,0};
static const unsigned long CLOCK_ANIM_MS = 250;
static const int CLOCK_SLIDE_PX = 14;

// Blink (independent, overlays between actions)
static unsigned long nextBlinkMs = 0;
static unsigned long blinkEndMs  = 0;
static const unsigned long BLINK_DURATION_MS  = 500;
static const unsigned long BLINK_INTERVAL_MIN = 3000;
static const unsigned long BLINK_INTERVAL_MAX = 7000;

// Yawn (rising edge: sleep → awake)
static bool wasSleeping    = false;
static unsigned long yawnEndMs = 0;
static const unsigned long YAWN_DURATION_MS = 1500;

// ---- Random action system ----
enum CatAction {
    ACT_IDLE,   // just standing
    ACT_WAVE,   // waving (WAVEA/WAVEB alternating)
    ACT_LOVE,   // happy face
    ACT_NAP,    // short nap (sleep + zzz, then yawn on wake)
    ACT_WALK    // walk right and back
};

static CatAction currentAction = ACT_IDLE;
static unsigned long actionStartMs = 0;
static unsigned long actionDurationMs = 0;
static unsigned long nextActionMs = 0;

// Action durations (ms)
static const unsigned long WAVE_DURATION   = 2000;
static const unsigned long LOVE_DURATION   = 2500;
static const unsigned long NAP_DURATION_MIN = 8000;
static const unsigned long NAP_DURATION_MAX = 15000;
static const unsigned long WALK_DURATION_MIN = 4000;
static const unsigned long WALK_DURATION_MAX = 6000;

// Walk state
static int walkDistance = 0;  // how far right to walk (px)

// Interval between actions (ms)
static const unsigned long ACTION_INTERVAL_MIN = 8000;
static const unsigned long ACTION_INTERVAL_MAX = 25000;

static void pickRandomAction(unsigned long now) {
    int r = random(100);
    if (r < 25) {
        currentAction = ACT_WAVE;
        actionDurationMs = WAVE_DURATION;
    } else if (r < 45) {
        currentAction = ACT_LOVE;
        actionDurationMs = LOVE_DURATION;
    } else if (r < 60) {
        currentAction = ACT_NAP;
        actionDurationMs = random(NAP_DURATION_MIN, NAP_DURATION_MAX);
    } else if (r < 80) {
        currentAction = ACT_WALK;
        actionDurationMs = random(WALK_DURATION_MIN, WALK_DURATION_MAX);
        walkDistance = random(20, 45);
    } else {
        // Stay idle for this round
        currentAction = ACT_IDLE;
        actionDurationMs = 0;
    }
    actionStartMs = now;
    nextActionMs = now + actionDurationMs + random(ACTION_INTERVAL_MIN, ACTION_INTERVAL_MAX);
}

// ---- Butterflies ----
static const int MAX_BUTTERFLIES = 2;
static const unsigned long BTF_FLAP_MS    = 140;
static const unsigned long BTF_SPAWN_MIN  = 30000;
static const unsigned long BTF_SPAWN_MAX  = 80000;

struct Butterfly {
    float x;
    float baseY;
    float vx;
    uint16_t age;
    bool active;
};
static Butterfly btfs[MAX_BUTTERFLIES];
static unsigned long nextBtfAt = 0;

// ---- Zzz ----
static const int MAX_ZS = 4;
static const int Z_SCALE = 1;
static const unsigned long Z_SPAWN_MS = 1400;

static const uint8_t Z_BITMAP[9][9] = {
    {1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0,0},
    {0,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1},
};

struct Zzz {
    float x, y;
    float vx, vy;
    uint16_t age;
    bool active;
};
static Zzz zzs[MAX_ZS];
static unsigned long lastZSpawn = 0;

// ---- Hearts ----
static const int MAX_HEARTS = 5;
static const unsigned long HEART_SPAWN_MS = 400;

struct Heart {
    float x, y;
    float vx, vy;
    uint16_t age;
    bool active;
    uint8_t size;  // 0=small, 1=medium, 2=large
};
static Heart hearts[MAX_HEARTS];
static unsigned long lastHeartSpawn = 0;

// Error sprite scaled to same size as cat
static const int ERR_DRAW_W = CAT_DRAW_W;
static const int ERR_DRAW_H = CAT_DRAW_H;

// ---- Drawing helpers (pixel-a-pixel, sem pushImage, sem setSwapBytes) ----

static void drawErrorFrame(int ox, int oy) {
    for (int dy = 0; dy < ERR_DRAW_H; dy++) {
        int sy = dy * ERROR_FRAME_H / ERR_DRAW_H;
        for (int dx = 0; dx < ERR_DRAW_W; dx++) {
            int sx = dx * ERROR_FRAME_W / ERR_DRAW_W;
            uint16_t p = pgm_read_word(&error_frames[0][sy * ERROR_FRAME_W + sx]);
            if (p == ERROR_TRANSPARENT) continue;
            fb.drawPixel(ox + dx, oy + dy, p);
        }
    }
}

static void drawCatFrame(int ox, int oy, int frameIdx, bool flip = false) {
    for (int dy = 0; dy < CAT_DRAW_H; dy++) {
        int sy = dy * CAT_FRAME_H / CAT_DRAW_H;
        for (int dx = 0; dx < CAT_DRAW_W; dx++) {
            int sx = dx * CAT_FRAME_W / CAT_DRAW_W;
            uint16_t p = pgm_read_word(&cat_frames[frameIdx][sy * CAT_FRAME_W + sx]);
            if (p == CAT_TRANSPARENT) continue;
            int px = flip ? (CAT_DRAW_W - 1 - dx) : dx;
            fb.drawPixel(ox + px, oy + dy, p);
        }
    }
}

// Draw butterfly scaled down (~75%)
static const int BTF_DRAW_W = BTF_FRAME_W * 3 / 4;
static const int BTF_DRAW_H = BTF_FRAME_H * 3 / 4;

static void drawButterflyFrame(int ox, int oy, int frameIdx, bool flip) {
    for (int dy = 0; dy < BTF_DRAW_H; dy++) {
        int sy = dy * BTF_FRAME_H / BTF_DRAW_H;
        for (int dx = 0; dx < BTF_DRAW_W; dx++) {
            int sx = dx * BTF_FRAME_W / BTF_DRAW_W;
            uint16_t p = pgm_read_word(&btf_frames[frameIdx][sy * BTF_FRAME_W + sx]);
            if (p == BTF_TRANSPARENT) continue;
            int px = flip ? (BTF_DRAW_W - 1 - dx) : dx;
            fb.drawPixel(ox + px, oy + dy, p);
        }
    }
}

// ---- Butterfly spawn/update/draw ----

static void spawnButterfly() {
    for (int i = 0; i < MAX_BUTTERFLIES; i++) {
        if (!btfs[i].active) {
            bool fromLeft = random(2);
            btfs[i].active = true;
            btfs[i].x     = fromLeft ? (float)-BTF_DRAW_W : (float)(W + 5);
            btfs[i].baseY  = -20.0f + random(0, 80);
            btfs[i].vx    = (fromLeft ? 1.0f : -1.0f) * (0.4f + random(0, 30) * 0.01f);
            btfs[i].age   = 0;
            return;
        }
    }
}

static void updateAndDrawButterflies() {
    const int framesPerFlap = BTF_FLAP_MS / 16;
    for (int i = 0; i < MAX_BUTTERFLIES; i++) {
        if (!btfs[i].active) continue;
        btfs[i].x += btfs[i].vx;
        btfs[i].age++;
        if (btfs[i].x < -BTF_DRAW_W - 5 || btfs[i].x > W + 5) {
            btfs[i].active = false;
            continue;
        }
        int yOsc = (int)(sinf(btfs[i].age * 0.06f) * 10.0f);
        int y = (int)btfs[i].baseY + yOsc;
        int frameIdx = (btfs[i].age / framesPerFlap) % 2;
        bool flip = (btfs[i].vx > 0);
        drawButterflyFrame((int)btfs[i].x, y, frameIdx, flip);
    }
}

// ---- Zzz spawn/update/draw ----

static void spawnZ(int headX, int headY) {
    for (int i = 0; i < MAX_ZS; i++) {
        if (!zzs[i].active) {
            zzs[i].active = true;
            zzs[i].x  = headX + random(-6, 7);
            zzs[i].y  = headY;
            zzs[i].vx = random(-10, 11) * 0.02f;
            zzs[i].vy = -0.4f - random(0, 30) * 0.01f;
            zzs[i].age = 0;
            return;
        }
    }
}

static void drawZ(int x, int y) {
    for (int row = 0; row < 9; row++) {
        for (int col = 0; col < 9; col++) {
            if (!Z_BITMAP[row][col]) continue;
            fb.fillRect(x + col * Z_SCALE, y + row * Z_SCALE,
                        Z_SCALE, Z_SCALE, COL_TEXT);
        }
    }
}

static void updateAndDrawZs() {
    for (int i = 0; i < MAX_ZS; i++) {
        if (!zzs[i].active) continue;
        zzs[i].x += zzs[i].vx;
        zzs[i].y += zzs[i].vy;
        zzs[i].age++;
        float wx = sinf(zzs[i].age * 0.06f) * 1.2f;
        if (zzs[i].y < -16 || zzs[i].x > W + 10) {
            zzs[i].active = false;
            continue;
        }
        drawZ((int)(zzs[i].x + wx), (int)zzs[i].y);
    }
}

// ---- Heart spawn/update/draw ----

static void drawHeart(int cx, int cy, int size, uint16_t col) {
    // Procedural heart shape scaled by size (0=3px, 1=5px, 2=7px)
    int r = size + 2;  // radius: 2, 3, 4
    // Two circles for top bumps
    fb.fillCircle(cx - r / 2, cy, r, col);
    fb.fillCircle(cx + r / 2, cy, r, col);
    // Triangle for bottom point
    fb.fillTriangle(cx - r - r / 2, cy + 1,
                    cx + r + r / 2, cy + 1,
                    cx, cy + r * 2, col);
}

static void spawnHeart(int headX, int headY) {
    for (int i = 0; i < MAX_HEARTS; i++) {
        if (!hearts[i].active) {
            hearts[i].active = true;
            hearts[i].x  = headX + random(-15, 16);
            hearts[i].y  = headY;
            hearts[i].vx = random(-15, 16) * 0.03f;
            hearts[i].vy = -0.5f - random(0, 40) * 0.01f;
            hearts[i].age = 0;
            hearts[i].size = random(3);
            return;
        }
    }
}

static void updateAndDrawHearts() {
    for (int i = 0; i < MAX_HEARTS; i++) {
        if (!hearts[i].active) continue;
        hearts[i].x += hearts[i].vx;
        hearts[i].y += hearts[i].vy;
        hearts[i].age++;
        float wobble = sinf(hearts[i].age * 0.08f) * 1.5f;
        if (hearts[i].y < -10 || hearts[i].age > 120) {
            hearts[i].active = false;
            continue;
        }
        // Alternate between pink and highlight
        uint16_t col = (hearts[i].age % 20 < 10) ? COL_HEART : COL_HEART_HI;
        drawHeart((int)(hearts[i].x + wobble), (int)hearts[i].y,
                  hearts[i].size, col);
    }
}

// ---- Weather icons (procedural, ~28x24 bounding box) ----
static const int ICON_W = 28;
static const int ICON_H = 24;

static void drawCloudShape(int x, int y, uint16_t col) {
    fb.fillCircle(x + 5, y + 9, 5, col);
    fb.fillCircle(x + 13, y + 5, 7, col);
    fb.fillCircle(x + 21, y + 9, 5, col);
    fb.fillRect(x + 1, y + 9, 24, 7, col);
}

static void drawWeatherIcon(int x, int y, WeatherIcon icon) {
    switch (icon) {
        case ICON_CLEAR: {
            int cx = x + ICON_W / 2;
            int cy = y + ICON_H / 2;
            fb.fillCircle(cx, cy, 7, COL_SUN);
            for (int a = 0; a < 360; a += 45) {
                float rad = a * PI / 180.0f;
                int x1 = cx + (int)(9 * cosf(rad));
                int y1 = cy + (int)(9 * sinf(rad));
                int x2 = cx + (int)(11 * cosf(rad));
                int y2 = cy + (int)(11 * sinf(rad));
                fb.drawLine(x1, y1, x2, y2, COL_SUN);
            }
            break;
        }
        case ICON_PARTLY_CLOUDY: {
            // Sun peeking upper-left
            fb.fillCircle(x + 9, y + 6, 5, COL_SUN);
            for (int a = 180; a < 360; a += 45) {
                float rad = a * PI / 180.0f;
                int x1 = x + 9 + (int)(7 * cosf(rad));
                int y1 = y + 6 + (int)(7 * sinf(rad));
                int x2 = x + 9 + (int)(9 * cosf(rad));
                int y2 = y + 6 + (int)(9 * sinf(rad));
                fb.drawLine(x1, y1, x2, y2, COL_SUN);
            }
            // Cloud overlapping lower-right
            drawCloudShape(x + 1, y + 6, COL_CLOUD);
            break;
        }
        case ICON_CLOUDY:
            drawCloudShape(x + 1, y + 4, COL_CLOUD);
            break;
        case ICON_RAIN:
            drawCloudShape(x + 1, y + 1, COL_CLOUD);
            fb.drawLine(x + 6,  y + 17, x + 4,  y + 21, COL_RAIN);
            fb.drawLine(x + 13, y + 17, x + 11, y + 21, COL_RAIN);
            fb.drawLine(x + 20, y + 17, x + 18, y + 21, COL_RAIN);
            break;
        case ICON_SNOW:
            drawCloudShape(x + 1, y + 1, COL_CLOUD);
            fb.fillCircle(x + 6,  y + 20, 2, COL_SNOW_DOT);
            fb.fillCircle(x + 13, y + 20, 2, COL_SNOW_DOT);
            fb.fillCircle(x + 20, y + 20, 2, COL_SNOW_DOT);
            break;
    }
}

static void drawScene() {
    // Shadow ellipse under cat (acompanha breathing)
    int sx = CAT_OX + CAT_DRAW_W / 2;
    int sy = CAT_OY + CAT_DRAW_H - 10;
    fb.fillEllipse(sx, sy, 28, 3, COL_SHADOW);
}

// ---- State screens ----

static void drawConnecting() {
    fb.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_IDLE);

    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Conectando WiFi", TEXT_AREA_CX, 50, 2);

    int dots = (millis() / 500) % 4;
    String d = "";
    for (int i = 0; i < dots; i++) d += ".";
    fb.drawString(d, TEXT_AREA_CX, 70, 2);
}

static void drawResetHint() {
    // Texto à direita + seta apontando pro botão BOOT (canto inferior direito)
    fb.setTextDatum(TR_DATUM);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    int ty = H - 20;
    int tx = W - 16;
    fb.drawString("Aguarde ou segure 10s p/ reconfig.", tx, ty, 2);
    int ax = W - 8, ay = ty + 8;
    fb.fillTriangle(ax + 7, ay, ax + 1, ay - 5, ax + 1, ay + 5, COL_TEXT_DIM);
}

static void drawError() {
    fb.fillSprite(COL_BG);
    drawScene();
    drawErrorFrame(CAT_OX, CAT_OY);

    fb.setTextDatum(TC_DATUM);

    fb.setFreeFont(&FreeSansBold12pt7b);
    fb.setTextColor(COL_ALERT_FG, COL_BG);
    fb.drawString("WiFi desconectado", TEXT_AREA_CX, 55);

    fb.setFreeFont(&FreeSansBold9pt7b);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Reconectando...", TEXT_AREA_CX, 85);

    fb.setTextFont(1);  // reset
    drawResetHint();
}

static const unsigned long API_WAVE_MS = 200;  // alternate every 200ms

static const uint16_t COL_GREEN = 0x07E0;  // green for WiFi OK

static void drawApiError() {
    fb.fillSprite(COL_BG);
    drawScene();
    drawErrorFrame(CAT_OX, CAT_OY);

    fb.setTextDatum(TC_DATUM);

    // Title
    fb.setFreeFont(&FreeSansBold12pt7b);
    fb.setTextColor(COL_ALERT_FG, COL_BG);
    fb.drawString("Erro na API", TEXT_AREA_CX, 30);

    // WiFi status with green check
    fb.setFreeFont(&FreeSansBold9pt7b);
    fb.setTextColor(COL_GREEN, COL_BG);
    fb.drawString("WiFi Conectado", TEXT_AREA_CX, 65);
    // Green dot as status indicator
    fb.fillCircle(TEXT_AREA_CX - fb.textWidth("WiFi Conectado") / 2 - 8, 70, 4, COL_GREEN);

    // Subtitle (thin modern font)
    fb.setFreeFont(&FreeSans9pt7b);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Servidor inacessivel", TEXT_AREA_CX, 100);

    // "Tentando novamente" + spinner to the right
    String retryStr = "Tentando novamente";
    int retryW = fb.textWidth(retryStr);
    int totalW = retryW + 16;  // text + gap + spinner
    int retryX = TEXT_AREA_CX - totalW / 2;
    fb.setTextDatum(TL_DATUM);
    fb.drawString(retryStr, retryX, 120);

    // Spinner to the right of text
    int spinCX = retryX + retryW + 10;
    int spinCY = 126;
    int spinR = 5;
    float angle = (millis() % 1000) * 2.0f * PI / 1000.0f;
    for (int i = 0; i < 6; i++) {
        float a = angle + i * 0.3f;
        int px = spinCX + (int)(spinR * cosf(a));
        int py = spinCY + (int)(spinR * sinf(a));
        uint8_t brightness = 255 - i * 40;
        uint16_t col = ((brightness >> 3) << 11) | ((brightness >> 2) << 5) | (brightness >> 3);
        fb.fillCircle(px, py, 1, col);
    }

    fb.setTextFont(1);  // reset
}

static const uint16_t COL_RESET_BG = 0xC000;  // vermelho escuro
static const uint16_t COL_RESET_BAR = 0xF800; // vermelho

static void drawResetOverlay() {
    if (g_resetPressMs < 3000) return;

    int secsLeft = (int)((10000 - g_resetPressMs) / 1000) + 1;
    if (secsLeft < 0) secsLeft = 0;

    // Barra de progresso no topo
    float progress = (float)(g_resetPressMs - 3000) / 7000.0f;
    if (progress > 1.0f) progress = 1.0f;
    int barW = (int)(W * progress);
    fb.fillRect(0, 0, barW, 4, COL_RESET_BAR);

    // Texto centralizado
    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(COL_RESET_BAR, COL_BG);
    fb.drawString("Resetar WiFi: " + String(secsLeft) + "s", CX, H - 20, 2);
}

static const int PORTAL_TEXT_X = 135;

static void drawPortal() {
    fb.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_IDLE, true);

    int x = PORTAL_TEXT_X;
    int y = 8;

    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(COL_ALERT_FG, COL_BG);
    fb.drawString("Configurar WiFi", x, y, 2);
    y += 22;

    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Rede:", x, y, 2);
    y += 15;
    fb.setTextColor(COL_TEXT, COL_BG);
    fb.drawString(portalGetAPSSID(), x, y, 2);
    y += 20;

    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Senha:", x, y, 2);
    y += 15;
    fb.setTextColor(COL_TEXT, COL_BG);
    fb.drawString(portalGetAPPassword(), x, y, 2);
    y += 20;

    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Abra no navegador:", x, y, 2);
    y += 15;
    fb.setTextColor(COL_TEXT, COL_BG);
    fb.drawString(portalGetAPIP(), x, y, 2);
}

static void drawWifiRetry(int secsLeft) {
    fb.fillSprite(COL_BG);
    drawScene();
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_ALERT);

    int x = PORTAL_TEXT_X;
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(COL_ALERT_FG, COL_BG);
    fb.drawString("Erro de conexao", x, 30, 4);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Tente novamente em", x, 65, 2);
    fb.setTextColor(COL_TEXT, COL_BG);
    fb.drawString(String(secsLeft) + " segundos", x, 85, 2);

    drawResetHint();
}

// ---- Public API ----

// ---- Button reading ----
static bool btnBootPrev = false;
static bool btnKeyPrev  = false;

static void forceAction(CatAction action, unsigned long duration) {
    currentAction = action;
    actionStartMs = millis();
    actionDurationMs = duration;
    nextActionMs = millis() + duration + random(ACTION_INTERVAL_MIN, ACTION_INTERVAL_MAX);
    // If waking from nap, cancel yawn
    wasSleeping = false;
}

static void updateButtons(AppState appState) {
    bool bootDown = (digitalRead(BTN_BOOT) == LOW);
    bool keyDown  = (digitalRead(BTN_KEY) == LOW);

    // BOOT button: pet → LOVE
    if (bootDown && !btnBootPrev) {
        lastInteraction = millis();
        if (appState == STATE_ALERT) {
            stateDismissAlert();
        } else {
            forceAction(ACT_LOVE, 3000);
        }
    }

    // KEY button: play → WAVE
    if (keyDown && !btnKeyPrev) {
        lastInteraction = millis();
        if (appState == STATE_ALERT) {
            stateDismissAlert();
        } else {
            forceAction(ACT_WAVE, 3000);
        }
    }

    btnBootPrev = bootDown;
    btnKeyPrev  = keyDown;
}

// ---- Determine which cat frame to show ----
static int chooseCatFrame(unsigned long now, AppState appState) {
    // ALERT from API has highest priority
    if (appState == STATE_ALERT) return CAT_FRAME_ALERT;

    // Trigger new random action?
    if (now >= nextActionMs) {
        // If finishing a nap, yawn first
        if (currentAction == ACT_NAP) {
            wasSleeping = true;
            yawnEndMs = now + YAWN_DURATION_MS;
        }
        pickRandomAction(now);
    }

    // Yawning (waking up from nap)
    if (wasSleeping && currentAction != ACT_NAP) {
        if (now < yawnEndMs) return CAT_FRAME_YAWN;
        wasSleeping = false;
    }

    // Current action
    bool inAction = (now - actionStartMs) < actionDurationMs;
    if (inAction) {
        switch (currentAction) {
            case ACT_WAVE: {
                int waveFrame = ((now / 200) % 2 == 0) ? CAT_FRAME_WAVEA : CAT_FRAME_WAVEB;
                return waveFrame;
            }
            case ACT_LOVE:
                return CAT_FRAME_LOVE;
            case ACT_NAP:
                return CAT_FRAME_SLEEP;
            case ACT_WALK: {
                int walkFrame = ((now / 250) % 2 == 0) ? CAT_FRAME_WAVEA : CAT_FRAME_WAVEB;
                return walkFrame;
            }
            default:
                break;
        }
    }

    // Blink timer (independent, fires during idle)
    if (now < blinkEndMs) return CAT_FRAME_BLINK;
    if (now >= nextBlinkMs) {
        blinkEndMs = now + BLINK_DURATION_MS;
        nextBlinkMs = now + BLINK_INTERVAL_MIN + random(BLINK_INTERVAL_MAX - BLINK_INTERVAL_MIN);
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
    nextActionMs = millis() + random(5000, 12000);
    nextBtfAt = millis() + random(2000, 6000);
    for (int i = 0; i < MAX_BUTTERFLIES; i++) btfs[i].active = false;
    for (int i = 0; i < MAX_ZS; i++) zzs[i].active = false;
    for (int i = 0; i < MAX_HEARTS; i++) hearts[i].active = false;
    randomSeed(esp_random());
}

void renderFrame(AppState state) {
    updateButtons(state);

    if (state == STATE_CONNECTING) {
        drawConnecting();
        drawResetOverlay();
        fb.pushSprite(0, 0);
        return;
    }

    if (state == STATE_PORTAL) {
        drawPortal();
        drawResetOverlay();
        fb.pushSprite(0, 0);
        return;
    }

    if (state == STATE_ERROR) {
        drawError();
        drawResetOverlay();
        fb.pushSprite(0, 0);
        return;
    }

    if (state == STATE_API_ERROR) {
        drawApiError();
        drawResetOverlay();
        fb.pushSprite(0, 0);
        return;
    }

    // IDLE or ALERT
    unsigned long now = millis();
    int frame = chooseCatFrame(now, state);
    bool sleeping = (frame == CAT_FRAME_SLEEP);

    // Breathing bob when sleeping
    int bob = 0;
    if (sleeping) {
        bob = (int)(sinf(now * 0.0018f) * 2.0f);
    }

    // Walk offset: cat moves right then back with ease-in-out
    int walkOX = 0;
    int walkBob = 0;
    bool walkFlip = false;
    bool walking = (currentAction == ACT_WALK) && ((now - actionStartMs) < actionDurationMs);
    if (walking) {
        float t = (float)(now - actionStartMs) / (float)actionDurationMs;
        float phase = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
        phase = phase * phase * (3.0f - 2.0f * phase);  // ease-in-out
        walkOX = (int)(phase * walkDistance);
        walkBob = (int)(sinf(now * 0.012f) * 2.0f);
        walkFlip = (t > 0.5f);
    }

    int catX = CAT_OX + walkOX;
    int catY = CAT_OY + bob + walkBob;

    // Spawn butterflies periodically
    if (now >= nextBtfAt) {
        spawnButterfly();
        nextBtfAt = now + random(BTF_SPAWN_MIN, BTF_SPAWN_MAX);
    }

    // Spawn hearts during LOVE action
    bool loving = (frame == CAT_FRAME_LOVE);
    if (loving && (now - lastHeartSpawn >= HEART_SPAWN_MS)) {
        spawnHeart(catX + CAT_DRAW_W / 2, catY + CAT_DRAW_H / 4);
        lastHeartSpawn = now;
    }

    // Spawn Zzz when sleeping
    if (sleeping && (now - lastZSpawn >= Z_SPAWN_MS)) {
        spawnZ(catX + CAT_DRAW_W / 2 - (9 * Z_SCALE) / 2, catY + CAT_DRAW_H / 4);
        lastZSpawn = now;
    }

    fb.fillSprite(COL_BG);

    // Butterflies behind cat
    updateAndDrawButterflies();

    // Shadow follows cat
    if (!sleeping) {
        int sx = catX + CAT_DRAW_W / 2;
        int sy = CAT_OY + CAT_DRAW_H - 10;
        fb.fillEllipse(sx, sy, 28, 3, COL_SHADOW);
    }
    drawCatFrame(catX, catY, frame, walkFlip);

    // Zzz and hearts in front of cat
    updateAndDrawZs();
    updateAndDrawHearts();

    // Right side: alert text OR clock
    if (state == STATE_ALERT) {
        EventData ev = stateGetEvent();
        if (ev.valid && ev.title.length() > 0) {
            String title = ev.title;
            if (title.length() > 18) title = title.substring(0, 17) + "..";
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(COL_ALERT_FG, COL_BG);
            fb.drawString(title, TEXT_AREA_CX, 50, 4);
            int mins = stateMinutesUntilEvent();
            if (mins >= 0) {
                fb.setTextColor(COL_TEXT, COL_BG);
                fb.drawString(mins == 0 ? "Agora" : "em " + String(mins) + " min", TEXT_AREA_CX, 80, 2);
            }
        }
    } else {
        if (timeSyncIsReady()) {
            String timeStr = timeSyncGetHHMM();

            // Detect digit changes and trigger animation
            for (int i = 0; i < 5; i++) {
                if (timeStr[i] != prevClock[i]) {
                    clockAnimMs[i] = now;
                    prevClock[i] = timeStr[i];
                }
            }

            // Draw each character with individual slide animation
            int totalW = fb.textWidth(timeStr, 7);
            int penX = TEXT_AREA_CX - totalW / 2 - 6;  // nudge left for visual balance
            fb.setTextDatum(TL_DATUM);

            for (int i = 0; i < 5; i++) {
                char ch[2] = {timeStr[i], '\0'};
                int chW = fb.textWidth(ch, 7);

                int yOff = 0;
                if (clockAnimMs[i] > 0 && now - clockAnimMs[i] < CLOCK_ANIM_MS) {
                    float t = (float)(now - clockAnimMs[i]) / CLOCK_ANIM_MS;
                    t = 1.0f - (1.0f - t) * (1.0f - t);  // ease-out
                    yOff = (int)((1.0f - t) * CLOCK_SLIDE_PX);
                }

                fb.setTextColor(COL_TEXT);
                fb.drawString(ch, penX, 30 + yOff, 7);
                penX += chW;
            }

            // Date (static, no animation)
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(COL_TEXT_DIM);
            fb.drawString(timeSyncGetDateStr(), TEXT_AREA_CX, 85, 4);

            // Temperature + weather icon: "22°C ☀" or loading spinner
            if (!weatherIsReady()) {
                // Spinner while loading temperature
                int spinCX = TEXT_AREA_CX;
                int spinCY = 124;
                int spinR = 6;
                float angle = (millis() % 1000) * 2.0f * PI / 1000.0f;
                for (int i = 0; i < 8; i++) {
                    float a = angle + i * 0.35f;
                    int px = spinCX + (int)(spinR * cosf(a));
                    int py = spinCY + (int)(spinR * sinf(a));
                    uint8_t brightness = 200 - i * 22;
                    uint16_t col = ((brightness >> 3) << 11) | ((brightness >> 2) << 5) | (brightness >> 3);
                    fb.fillCircle(px, py, 1, col);
                }
            } else if (weatherIsReady()) {
                String numStr = String(weatherGetTemp());
                fb.setFreeFont(&FreeSansBold12pt7b);
                int numW = fb.textWidth(numStr);
                int degCW = fb.textWidth("C");
                int totalW = numW + 8 + degCW + 6 + ICON_W;  // num + °C + gap + icon
                int startX = TEXT_AREA_CX - totalW / 2 + 6;  // nudge right for visual balance
                int tempY = 117;

                // Temperature number
                fb.setTextDatum(TL_DATUM);
                fb.setTextColor(COL_TEXT);
                fb.drawString(numStr, startX, tempY);

                // Degree circle + "C"
                int degX = startX + numW + 2;
                fb.drawCircle(degX + 2, tempY + 1, 2, COL_TEXT);
                fb.drawString("C", degX + 6, tempY);

                fb.setTextFont(1);  // reset to built-in font

                // Weather icon (after text)
                int iconX = startX + numW + 8 + degCW + 6;
                drawWeatherIcon(iconX, tempY - 2, weatherGetIcon());
            }
        } else {
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(COL_TEXT_DIM);
            fb.drawString("--:--", TEXT_AREA_CX, 30, 7);
            fb.drawString("Sincronizando...", TEXT_AREA_CX, 85, 2);
        }
    }

    drawResetOverlay();
    fb.pushSprite(0, 0);
}

void renderWifiRetry(int secsLeft) {
    drawWifiRetry(secsLeft);
    drawResetOverlay();
    fb.pushSprite(0, 0);
}
