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
static const uint16_t COL_BG        = 0x0000;  // preto
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
static const int CLOCK_MAX_CHARS = 8;  // "12:30PM\0"
static char prevClock[CLOCK_MAX_CHARS + 1] = "";
static unsigned long clockAnimMs[CLOCK_MAX_CHARS] = {};
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

// ---- Text helpers ----

// Strip UTF-8 accents to ASCII equivalents for TFT fonts
static String stripAccents(const String& s) {
    String out;
    out.reserve(s.length());
    for (unsigned int i = 0; i < s.length(); i++) {
        uint8_t c = (uint8_t)s[i];
        if (c == 0xC3 && i + 1 < s.length()) {
            uint8_t n = (uint8_t)s[i + 1];
            i++;
            // Lowercase: à-å→a, è-ë→e, ì-ï→i, ò-ö→o, ù-ü→u, ñ→n, ý→y
            if (n >= 0xA0 && n <= 0xA5) out += 'a';
            else if (n == 0xA7) out += 'c';  // ç
            else if (n >= 0xA8 && n <= 0xAB) out += 'e';
            else if (n >= 0xAC && n <= 0xAF) out += 'i';
            else if (n == 0xB1) out += 'n';  // ñ
            else if (n >= 0xB2 && n <= 0xB6) out += 'o';
            else if (n >= 0xB9 && n <= 0xBC) out += 'u';
            else if (n == 0xBD) out += 'y';
            // Uppercase
            else if (n >= 0x80 && n <= 0x85) out += 'A';
            else if (n == 0x87) out += 'C';
            else if (n >= 0x88 && n <= 0x8B) out += 'E';
            else if (n >= 0x8C && n <= 0x8F) out += 'I';
            else if (n == 0x91) out += 'N';
            else if (n >= 0x92 && n <= 0x96) out += 'O';
            else if (n >= 0x99 && n <= 0x9C) out += 'U';
            else if (n == 0x9D) out += 'Y';
            else out += '?';
        } else if (c < 0x80) {
            out += (char)c;
        }
        // Skip other multi-byte sequences
    }
    return out;
}

// Draw word-wrapped text centered, returns Y after last line
static int drawWrappedText(const String& text, int cx, int y, int maxW, int font, int lineH) {
    String remaining = text;
    while (remaining.length() > 0 && y < H - 10) {
        // Find how many chars fit in maxW
        String line = remaining;
        while (fb.textWidth(line, font) > maxW && line.length() > 0) {
            // Try to break at last space
            int lastSpace = line.lastIndexOf(' ');
            if (lastSpace > 0) {
                line = line.substring(0, lastSpace);
            } else {
                line = line.substring(0, line.length() - 1);
            }
        }
        fb.drawString(line, cx, y, font);
        y += lineH;
        remaining = remaining.substring(line.length());
        remaining.trim();
    }
    return y;
}

// Draw a small clock icon (circle + hands)
static void drawClockIcon(int cx, int cy, int r, uint16_t col) {
    fb.drawCircle(cx, cy, r, col);
    fb.drawLine(cx, cy, cx, cy - r + 2, col);       // 12 o'clock hand
    fb.drawLine(cx, cy, cx + r - 2, cy, col);        // 3 o'clock hand
}

// ---- Color remapping for customization ----

// Reference colors per region (RGB565) — must match web/src/data/catSprite.ts
static const uint16_t REGION_REF[6] = {
    0x0000,  // 0: outline (black)
    0x5981,  // 1: stripes (dark brown)
    0xD2C7,  // 2: body (orange-red)
    0xF56C,  // 3: belly (light orange)
    0xFFFF,  // 4: eyes (white)
    0xEED4,  // 5: nose (cream)
};
static const int NUM_REGIONS = 6;

// Color lookup table: maps original RGB565 → remapped RGB565
// Using a simple hash map (open addressing, 256 entries — fits ~132 unique colors)
static const int CLT_SIZE = 256;
static uint16_t cltKeys[CLT_SIZE];
static uint16_t cltVals[CLT_SIZE];
static bool     cltUsed[CLT_SIZE];  // track occupied slots (avoids 0xFFFF collision with white)
static bool     cltActive = false;
static String   cltFingerprint = "";  // detect changes

// RGB565 ↔ RGB888 conversions
static void rgb565ToRgb(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = ((c >> 11) & 0x1F) * 255 / 31;
    g = ((c >> 5) & 0x3F) * 255 / 63;
    b = (c & 0x1F) * 255 / 31;
}

static uint16_t rgbToRgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// RGB → HSL (0-1 range)
static void rgbToHsl(uint8_t ri, uint8_t gi, uint8_t bi, float& h, float& s, float& l) {
    float r = ri / 255.0f, g = gi / 255.0f, b = bi / 255.0f;
    float mx = max(max(r, g), b), mn = min(min(r, g), b);
    l = (mx + mn) / 2.0f;
    if (mx == mn) { h = s = 0; return; }
    float d = mx - mn;
    s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    if (mx == r) h = (g - b) / d + (g < b ? 6.0f : 0);
    else if (mx == g) h = (b - r) / d + 2.0f;
    else h = (r - g) / d + 4.0f;
    h /= 6.0f;
}

static float hue2rgb(float p, float q, float t) {
    if (t < 0) t += 1; if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 0.5f) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

static void hslToRgb(float h, float s, float l, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (s == 0) { r = g = b = (uint8_t)(l * 255); return; }
    float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    r = (uint8_t)(hue2rgb(p, q, h + 1.0f/3) * 255);
    g = (uint8_t)(hue2rgb(p, q, h) * 255);
    b = (uint8_t)(hue2rgb(p, q, h - 1.0f/3) * 255);
}

// Parse "#RRGGBB" hex string to RGB
static bool parseHex(const String& hex, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (hex.length() != 7 || hex[0] != '#') return false;
    long val = strtol(hex.c_str() + 1, nullptr, 16);
    r = (val >> 16) & 0xFF;
    g = (val >> 8) & 0xFF;
    b = val & 0xFF;
    return true;
}

// Classify an RGB565 pixel into a region by nearest reference color
static int classifyPixel(uint16_t p) {
    uint8_t pr, pg, pb;
    rgb565ToRgb(p, pr, pg, pb);
    int best = 0;
    int bestDist = 999999;
    for (int i = 0; i < NUM_REGIONS; i++) {
        uint8_t rr, rg, rb;
        rgb565ToRgb(REGION_REF[i], rr, rg, rb);
        int dr = (int)pr - rr, dg = (int)pg - rg, db = (int)pb - rb;
        int dist = dr*dr + dg*dg + db*db;
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    return best;
}

// Build color lookup table from custom colors
static void buildColorLUT(const CatColors& cc) {
    // Compute fingerprint to avoid rebuilding every frame
    String fp = cc.body + cc.stripes + cc.belly + cc.outline + cc.eyes + cc.nose;
    if (fp == cltFingerprint && cltActive) return;
    cltFingerprint = fp;

    // Parse target colors per region
    uint8_t targetR[NUM_REGIONS], targetG[NUM_REGIONS], targetB[NUM_REGIONS];
    float   targetH[NUM_REGIONS], targetS[NUM_REGIONS], targetL[NUM_REGIONS];
    const String* hexStrs[NUM_REGIONS] = { &cc.outline, &cc.stripes, &cc.body, &cc.belly, &cc.eyes, &cc.nose };

    for (int i = 0; i < NUM_REGIONS; i++) {
        parseHex(*hexStrs[i], targetR[i], targetG[i], targetB[i]);
        rgbToHsl(targetR[i], targetG[i], targetB[i], targetH[i], targetS[i], targetL[i]);
    }

    // Reference HSL per region
    float refH[NUM_REGIONS], refS[NUM_REGIONS], refL[NUM_REGIONS];
    for (int i = 0; i < NUM_REGIONS; i++) {
        uint8_t rr, rg, rb;
        rgb565ToRgb(REGION_REF[i], rr, rg, rb);
        rgbToHsl(rr, rg, rb, refH[i], refS[i], refL[i]);
    }

    // Delta per region
    float dh[NUM_REGIONS], ds[NUM_REGIONS], dl[NUM_REGIONS];
    for (int i = 0; i < NUM_REGIONS; i++) {
        dh[i] = targetH[i] - refH[i];
        ds[i] = targetS[i] - refS[i];
        dl[i] = targetL[i] - refL[i];
    }

    // Clear LUT
    memset(cltUsed, 0, sizeof(cltUsed));

    // Scan all frames for unique colors and build LUT
    for (int f = 0; f < CAT_FRAME_COUNT; f++) {
        for (int i = 0; i < CAT_FRAME_W * CAT_FRAME_H; i++) {
            uint16_t p = pgm_read_word(&cat_frames[f][i]);
            if (p == CAT_TRANSPARENT) continue;

            // Check if already in LUT
            uint16_t slot = p & (CLT_SIZE - 1);
            bool found = false;
            for (int j = 0; j < CLT_SIZE; j++) {
                uint16_t idx = (slot + j) & (CLT_SIZE - 1);
                if (cltUsed[idx] && cltKeys[idx] == p) { found = true; break; }
                if (!cltUsed[idx]) break;
            }
            if (found) continue;

            // Classify and remap
            int region = classifyPixel(p);
            uint8_t pr, pg, pb;
            rgb565ToRgb(p, pr, pg, pb);
            float ph, ps, pl;
            rgbToHsl(pr, pg, pb, ph, ps, pl);

            float nh = fmodf(ph + dh[region] + 1.0f, 1.0f);
            float ns = constrain(ps + ds[region], 0.0f, 1.0f);
            float nl = constrain(pl + dl[region], 0.0f, 1.0f);

            uint8_t nr, ng, nb;
            hslToRgb(nh, ns, nl, nr, ng, nb);
            uint16_t remapped = rgbToRgb565(nr, ng, nb);

            // Insert into LUT
            for (int j = 0; j < CLT_SIZE; j++) {
                uint16_t idx = (slot + j) & (CLT_SIZE - 1);
                if (!cltUsed[idx]) {
                    cltKeys[idx] = p;
                    cltVals[idx] = remapped;
                    cltUsed[idx] = true;
                    break;
                }
            }
        }
    }

    cltActive = true;
    Serial.println("[render] color LUT built");
}

// Look up a remapped color (returns original if not found or LUT not active)
static inline uint16_t remapColor(uint16_t p) {
    if (!cltActive) return p;
    uint16_t slot = p & (CLT_SIZE - 1);
    for (int j = 0; j < 8; j++) {  // max 8 probes
        uint16_t idx = (slot + j) & (CLT_SIZE - 1);
        if (!cltUsed[idx]) return p;
        if (cltKeys[idx] == p) return cltVals[idx];
    }
    return p;
}

static void drawCatFrame(int ox, int oy, int frameIdx, bool flip = false) {
    for (int dy = 0; dy < CAT_DRAW_H; dy++) {
        int sy = dy * CAT_FRAME_H / CAT_DRAW_H;
        for (int dx = 0; dx < CAT_DRAW_W; dx++) {
            int sx = dx * CAT_FRAME_W / CAT_DRAW_W;
            uint16_t p = pgm_read_word(&cat_frames[frameIdx][sy * CAT_FRAME_W + sx]);
            if (p == CAT_TRANSPARENT) continue;
            int px = flip ? (CAT_DRAW_W - 1 - dx) : dx;
            fb.drawPixel(ox + px, oy + dy, remapColor(p));
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

    // Update color LUT if custom colors are set
    CatColors cc = stateGetColors();
    if (cc.valid) {
        buildColorLUT(cc);
    } else if (cltActive) {
        cltActive = false;
        cltFingerprint = "";
    }

    // IDLE or ALERT
    unsigned long now = millis();
    int frame = chooseCatFrame(now, state);
    bool sleeping = (frame == CAT_FRAME_SLEEP);

    // Breathing bob when sleeping, bounce when alert
    int bob = 0;
    if (sleeping) {
        bob = (int)(sinf(now * 0.0018f) * 2.0f);
    } else if (state == STATE_ALERT) {
        float bounce = sinf(now * 0.006f);
        bob = (bounce > 0) ? (int)(-bounce * 5.0f) : 0;  // jump up only
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
            String title = stripAccents(ev.title);
            if (title.length() > 20) title = title.substring(0, 19) + "..";
            int areaW = TEXT_AREA_W - 8;

            // Title (top, orange)
            fb.setTextDatum(TC_DATUM);
            fb.setTextColor(COL_ALERT_FG, COL_BG);
            fb.drawString(title, TEXT_AREA_CX, 20, 4);

            // Description (centered, word-wrapped, subtle)
            int descY = 50;
            if (ev.description.length() > 0) {
                String desc = stripAccents(ev.description);
                fb.setTextColor(COL_TEXT, COL_BG);
                descY = drawWrappedText(desc, TEXT_AREA_CX, descY, areaW, 2, 16);
            }

            // Time + countdown at bottom with clock icon
            int mins = stateMinutesUntilEvent();
            if (mins >= 0) {
                String timeStr = timeSyncGetHHMM();
                String countdown = (mins == 0)
                    ? timeStr + " - Agora!"
                    : timeStr + " em " + String(mins) + " min";
                int font = 2;  // font between 2 (small) and 4 (large)
                fb.setFreeFont(&FreeSans9pt7b);
                int cw = fb.textWidth(countdown);
                int totalW = 12 + 4 + cw;  // icon + gap + text
                int startX = TEXT_AREA_CX - totalW / 2;
                int bottomY = H - 55;

                fb.setTextColor(COL_TEXT_DIM, COL_BG);
                drawClockIcon(startX + 5, bottomY + 6, 4, COL_TEXT_DIM);
                fb.setTextDatum(TL_DATUM);
                fb.drawString(countdown, startX + 14, bottomY);
                fb.setTextFont(1);  // reset
            }
        }
    } else {
        if (timeSyncIsReady()) {
            String timeStr = timeSyncGetHHMM();
            int len = timeStr.length();
            if (len > CLOCK_MAX_CHARS) len = CLOCK_MAX_CHARS;

            // Detect digit changes and trigger animation
            for (int i = 0; i < len; i++) {
                if (timeStr[i] != prevClock[i]) {
                    clockAnimMs[i] = now;
                    prevClock[i] = timeStr[i];
                }
            }
            prevClock[len] = '\0';

            // Draw each character with individual slide animation
            int totalW = fb.textWidth(timeStr, 7);
            int penX = TEXT_AREA_CX - totalW / 2;
            fb.setTextDatum(TL_DATUM);

            for (int i = 0; i < len; i++) {
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

    // IP address in bottom-right corner
    if (WiFi.status() == WL_CONNECTED) {
        fb.setTextDatum(BR_DATUM);
        fb.setTextColor(0x4228);  // very dim gray
        fb.drawString(WiFi.localIP().toString(), W - 4, H - 3, 1);
    }

    drawResetOverlay();
    fb.pushSprite(0, 0);
}

void renderWifiRetry(int secsLeft) {
    drawWifiRetry(secsLeft);
    drawResetOverlay();
    fb.pushSprite(0, 0);
}
