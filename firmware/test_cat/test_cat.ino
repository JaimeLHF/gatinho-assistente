/*
 * Gatinho Pet — Pixel Art Test
 * Board: LilyGo T-Display-S3 (ESP32-S3)
 *
 * Arduino IDE setup:
 * 1. Instale a lib "TFT_eSPI" pelo Library Manager
 * 2. Edite o User_Setup.h da TFT_eSPI (na pasta da lib) e
 *    descomente/configure para T-Display-S3, OU use o
 *    User_Setup_Select.h selecionando Setup206_LilyGo_T_Display_S3
 * 3. Board: "ESP32S3 Dev Module"
 *    USB CDC On Boot: Enabled
 *    Flash Size: 16MB
 *    PSRAM: OPI PSRAM
 */

#include <TFT_eSPI.h>

TFT_eSPI tft;

// Display 170x320 portrait
#define W 170
#define H 320

// ========== PALETA RGB565 ==========
#define BG         0x1926  // fundo azul escuro
#define COL_BLACK  0x0000
#define COL_BODY   0xE504  // laranja dourado
#define COL_STRIPE 0xB380  // marrom escuro
#define COL_CREAM  0xF71B  // creme claro
#define COL_PINK   0xEB2D  // rosa
#define COL_WHITE  0xFFFF

// 0=fundo, 1=preto, 2=corpo, 3=listra, 4=creme, 5=rosa
const uint16_t PALETTE[] = { BG, COL_BLACK, COL_BODY, COL_STRIPE, COL_CREAM, COL_PINK };

// ========== PIXEL ART: 16 x 21 — gatinho laranja tabby ==========
#define CAT_W 16
#define CAT_H 21
#define SCALE 7   // 16*7=112px, 21*7=147px

const uint8_t CAT[CAT_H][CAT_W] = {
    //                 Orelhas
    {0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0},  //  0  ponta orelhas
    {0,0,1,2,2,1,0,0,0,0,1,2,2,1,0,0},  //  1  orelhas
    {0,0,1,5,2,2,1,1,1,1,2,2,5,1,0,0},  //  2  interior rosa + topo cabeca
    //                 Cabeca
    {0,1,2,2,3,2,2,2,2,2,2,3,2,2,1,0},  //  3  cabeca + listras
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  4  cabeca
    {0,1,3,2,1,1,2,2,2,2,1,1,2,3,1,0},  //  5  olhos
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},  //  6  abaixo olhos
    {0,1,2,2,2,4,4,4,4,4,4,2,2,2,1,0},  //  7  focinho creme
    {1,0,1,3,2,4,4,5,5,4,4,2,3,1,0,1},  //  8  nariz + bigode
    {0,0,1,2,2,4,1,4,1,4,2,2,1,0,0,0},  //  9  boca
    {1,0,0,1,2,2,4,4,4,2,2,1,0,0,1,0},  // 10  queixo + bigode
    //                 Corpo
    {0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0},  // 11  pescoco
    {0,0,0,1,2,3,2,4,2,3,2,2,1,0,0,0},  // 12  peito + listras
    {0,0,1,2,2,2,3,4,3,2,2,2,2,1,0,0},  // 13  corpo
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 14  bracinhos (patinha creme)
    {0,1,4,1,2,2,2,4,2,2,2,2,1,4,1,0},  // 15  bracinhos
    {0,0,1,2,2,2,2,4,2,2,2,2,2,1,0,0},  // 16  corpo inferior
    //                 Pes + Rabo
    {0,0,1,2,2,2,2,2,2,2,2,2,2,1,1,0},  // 17  acima pes + inicio rabo
    {0,0,1,1,2,1,1,2,1,1,2,1,1,2,2,1},  // 18  patinhas + rabo
    {0,0,0,1,1,0,0,1,0,0,1,0,0,1,2,1},  // 19  solas + rabo
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},  // 20  ponta rabo
};

// ========== DESENHO ==========

void drawCat(int ox, int oy) {
    for (int row = 0; row < CAT_H; row++) {
        for (int col = 0; col < CAT_W; col++) {
            uint8_t idx = CAT[row][col];
            if (idx != 0) {
                tft.fillRect(ox + col * SCALE, oy + row * SCALE, SCALE, SCALE, PALETTE[idx]);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BG);

    // Backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    // Titulo
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COL_WHITE, BG);
    tft.drawString("Gatinho Pet", W / 2, 20, 4);

    // Gatinho centralizado
    int ox = (W - CAT_W * SCALE) / 2;  // (170-112)/2 = 29
    int oy = 60;
    drawCat(ox, oy);

    // Subtitulo
    tft.setTextColor(0xB596, BG);
    tft.drawString("Pixel Art Test", W / 2, oy + CAT_H * SCALE + 16, 2);

    // Footer
    tft.setTextColor(0x632C, BG);
    tft.drawString("T-Display-S3", W / 2, H - 20, 2);

    Serial.println("Gatinho desenhado!");
}

void loop() {
    delay(1000);
}
