#include <Arduino.h>
#include <TFT_eSPI.h>

// ============================================================
// Gatinho Pet — Test sketch: pixel art cat on T-Display-S3
// Build: pio run -e test-cat
// Upload: pio run -e test-cat --target upload
// ============================================================

TFT_eSPI tft;
TFT_eSprite sprite(&tft);

// Display dimensions (portrait)
static const int W = 170;
static const int H = 320;

// ---- Color Palette (RGB565) ----
static const uint16_t BG         = 0x1926;  // dark navy #192C4A
static const uint16_t COL_BLACK  = 0x0000;
static const uint16_t COL_BODY   = 0xE504;  // golden orange
static const uint16_t COL_STRIPE = 0xB380;  // dark orange/brown
static const uint16_t COL_CREAM  = 0xF71B;  // light cream
static const uint16_t COL_PINK   = 0xEB2D;  // pink (nose/inner ears)
static const uint16_t COL_WHITE  = 0xFFFF;

// Palette lookup: 0=transparent, 1=black, 2=body, 3=stripe, 4=cream, 5=pink
static const uint16_t PALETTE[] = {
    BG,          // 0 transparent (draws background)
    COL_BLACK,   // 1
    COL_BODY,    // 2
    COL_STRIPE,  // 3
    COL_CREAM,   // 4
    COL_PINK,    // 5
};

// ---- Pixel Art: 16 x 21 golden tabby cat ----
// Traced from the reference image (row 2, col 1 — the golden/orange tabby)
static const uint8_t CAT_W = 16;
static const uint8_t CAT_H = 21;

static const uint8_t CAT[21][16] = {
    //                   Ears
    {0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0},  //  0  ear tips
    {0,0,1,2,2,1,0,0,0,0,1,2,2,1,0,0},  //  1  ears outer
    {0,0,1,5,2,2,1,1,1,1,2,2,5,1,0,0},  //  2  ears inner + head top
    //                   Head
    {0,1,2,2,3,2,2,2,2,2,2,3,2,2,1,0},  //  3  head + stripe accents
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  4  head
    {0,1,3,2,1,1,2,2,2,2,1,1,2,3,1,0},  //  5  eyes (1,1 = eye dots)
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  6  below eyes
    {0,1,2,2,2,4,4,4,4,4,4,2,2,2,1,0},  //  7  muzzle cream
    {1,0,1,3,2,4,4,5,5,4,4,2,3,1,0,1},  //  8  nose + whisker dots
    {0,0,1,2,2,4,1,4,1,4,2,2,1,0,0,0},  //  9  mouth
    {1,0,0,1,2,2,4,4,4,2,2,1,0,0,1,0},  // 10  chin + whisker dots
    //                   Body
    {0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0},  // 11  neck
    {0,0,0,1,2,3,2,4,2,3,2,2,1,0,0,0},  // 12  upper body + stripes
    {0,0,1,2,2,2,3,4,3,2,2,2,2,1,0,0},  // 13  body
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 14  arms out (paws = cream)
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 15  arms
    {0,0,1,2,2,2,2,4,2,2,2,2,2,1,0,0},  // 16  lower body
    //                   Feet + Tail
    {0,0,1,2,2,2,2,2,2,2,2,2,2,1,1,0},  // 17  above feet + tail starts
    {0,0,1,1,2,1,1,2,1,1,2,1,1,2,2,1},  // 18  feet + tail
    {0,0,0,1,1,0,0,1,0,0,1,0,0,1,2,1},  // 19  soles + tail
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},  // 20  tail tip
};

// Scale factor: 16*7 = 112px wide, 21*7 = 147px tall — fits 170x320 nicely
static const uint8_t SCALE = 7;

// ---- Drawing functions ----

void drawPixelArtCat(int ox, int oy) {
    for (int row = 0; row < CAT_H; row++) {
        for (int col = 0; col < CAT_W; col++) {
            uint8_t idx = CAT[row][col];
            uint16_t color = PALETTE[idx];
            // Only draw non-transparent pixels for efficiency
            if (idx != 0) {
                sprite.fillRect(ox + col * SCALE, oy + row * SCALE, SCALE, SCALE, color);
            }
        }
    }
}

void drawScene() {
    sprite.fillSprite(BG);

    // Title
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(COL_WHITE);
    sprite.drawString("Gatinho Pet", W / 2, 20, 4);

    // Cat centered below title
    int catPixelW = CAT_W * SCALE;  // 112
    int catPixelH = CAT_H * SCALE;  // 147
    int ox = (W - catPixelW) / 2;
    int oy = 60;
    drawPixelArtCat(ox, oy);

    // Subtitle below cat
    sprite.setTextColor(0xB596); // warm gray
    sprite.drawString("Pixel Art Test", W / 2, oy + catPixelH + 16, 2);

    // Footer
    sprite.setTextColor(0x632C); // dim
    sprite.drawString("T-Display-S3 | 170x320", W / 2, H - 20, 2);

    sprite.pushSprite(0, 0);
}

void setup() {
    Serial.begin(115200);
    Serial.println("[test-cat] booting...");

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BG);

    // Backlight on
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    // Create full-screen sprite for flicker-free drawing
    sprite.createSprite(W, H);
    sprite.setSwapBytes(true);

    drawScene();
    Serial.println("[test-cat] drawn!");
}

void loop() {
    // Static image — nothing to do in loop
    delay(1000);
}
