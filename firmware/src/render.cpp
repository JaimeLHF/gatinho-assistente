#include "render.h"
#include "state.h"
#include "time_sync.h"
#include "wifi_portal.h"
#include "button_reset.h"
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
static const uint16_t COL_ALERT_FG  = 0xFCC8;  // laranja suave
static const uint16_t COL_HEART     = 0xFB14;  // heart pink
static const uint16_t COL_HEART_HI  = 0xFD9F;  // heart highlight

// Cat position: left side, vertically centered +4px
static const int CAT_OX = 30;
static const int CAT_OY = (H - CAT_FRAME_H * SCALE) / 2 + 4;

// Text area: right side of the cat
static const int TEXT_AREA_X  = 120;
static const int TEXT_AREA_W  = W - 120 - 8;
static const int TEXT_AREA_CX = TEXT_AREA_X + TEXT_AREA_W / 2;

// ---- Buttons ----
static const int BTN_BOOT = 0;   // GPIO 0 (BOOT)
static const int BTN_KEY  = 14;  // GPIO 14 (KEY)

// ---- Animation state ----
static unsigned long lastInteraction = 0;
static const unsigned long SLEEP_TIMEOUT_MS = 30000;  // 30s

// Clock digit slide animation
static char prevClock[6] = {'-','-',':','-','-','\0'};
static unsigned long clockAnimMs[5] = {0,0,0,0,0};
static const unsigned long CLOCK_ANIM_MS = 250;
static const int CLOCK_SLIDE_PX = 14;

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

// ---- Drawing helpers (pixel-a-pixel, sem pushImage, sem setSwapBytes) ----

static void drawCatFrame(int ox, int oy, int frameIdx, bool flip = false) {
    for (int y = 0; y < CAT_FRAME_H; y++) {
        for (int x = 0; x < CAT_FRAME_W; x++) {
            uint16_t p = pgm_read_word(&cat_frames[frameIdx][y * CAT_FRAME_W + x]);
            if (p == CAT_TRANSPARENT) continue;
            int dx = flip ? (CAT_FRAME_W - 1 - x) : x;
            if (SCALE == 1) {
                fb.drawPixel(ox + dx, oy + y, p);
            } else {
                fb.fillRect(ox + dx * SCALE, oy + y * SCALE, SCALE, SCALE, p);
            }
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
    drawCatFrame(CAT_OX, CAT_OY, CAT_FRAME_ALERT);

    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(COL_ALERT_FG, COL_BG);
    fb.drawString("Erro de conexao", TEXT_AREA_CX, 50, 2);
    fb.setTextColor(COL_TEXT_DIM, COL_BG);
    fb.drawString("Tentando novamente...", TEXT_AREA_CX, 70, 2);

    drawResetHint();
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
static bool btnPressed = false;

static void updateButtons(AppState appState) {
    bool down = (digitalRead(BTN_BOOT) == LOW || digitalRead(BTN_KEY) == LOW);
    if (down && !btnPressed) {
        lastInteraction = millis();
        if (appState == STATE_ALERT) {
            stateDismissAlert();
        }
    }
    btnPressed = down;
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
    nextBtfAt = millis() + random(2000, 6000);
    for (int i = 0; i < MAX_BUTTERFLIES; i++) btfs[i].active = false;
    for (int i = 0; i < MAX_ZS; i++) zzs[i].active = false;
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

    // IDLE or ALERT
    unsigned long now = millis();
    int frame = chooseCatFrame(now, state);
    bool sleeping = (frame == CAT_FRAME_SLEEP);

    // Spawn butterflies periodically
    if (now >= nextBtfAt) {
        spawnButterfly();
        nextBtfAt = now + random(BTF_SPAWN_MIN, BTF_SPAWN_MAX);
    }

    // Spawn Zzz when sleeping
    if (sleeping && (now - lastZSpawn >= Z_SPAWN_MS)) {
        int headX = CAT_OX + CAT_FRAME_W / 2 - (9 * Z_SCALE) / 2;
        int headY = CAT_OY + CAT_FRAME_H / 4;
        spawnZ(headX, headY);
        lastZSpawn = now;
    }

    // Breathing bob when sleeping (sin wave, ~1-2px vertical)
    int bob = 0;
    if (sleeping) {
        bob = (int)(sinf(now * 0.0018f) * 2.0f);
    }

    fb.fillSprite(COL_BG);

    // Butterflies behind cat
    updateAndDrawButterflies();

    if (!sleeping) drawScene();
    drawCatFrame(CAT_OX, CAT_OY + bob, frame);

    // Zzz in front of cat
    updateAndDrawZs();

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
            int penX = TEXT_AREA_CX - totalW / 2;
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
                fb.drawString(ch, penX, 45 + yOff, 7);
                penX += chW;
            }

            // Date (static, no animation)
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(COL_TEXT_DIM);
            fb.drawString(timeSyncGetDateStr(), TEXT_AREA_CX, 100, 4);
        } else {
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(COL_TEXT_DIM);
            fb.drawString("--:--", TEXT_AREA_CX, 50, 7);
            fb.drawString("Sincronizando...", TEXT_AREA_CX, 100, 2);
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
