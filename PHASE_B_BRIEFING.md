# FASE B — Migração de Sprites Bonitos pro Firmware PlatformIO

> **Status:** MVP da Fase A funcionando (ESP32 conectado, polling de eventos, alertas exibidos).
> **Objetivo desta fase:** Substituir o gato feinho atual por sprites animados bonitos.

---

## 📋 Contexto rápido

- **Projeto:** `~/projetos/gatinho-assistente`
- **Firmware atual:** `firmware/src/` (PlatformIO, env `t-display-s3`)
- **Hardware:** LilyGo T-Display-S3 (ESP32-S3, tela 320×170)
- **Estado funcional confirmado:** firmware atual conecta WiFi, faz polling na API, exibe hora/data/alertas
- **NÃO MEXER em:** `network.cpp/h`, `state.cpp/h`, `config.h` (a menos que estritamente necessário)
- **MEXER em:** `render.cpp/h`, `main.cpp`, **adicionar** `firmware/src/sprites/`

---

## 🎯 Decisões fechadas (NÃO discutir, executar)

1. **Sprites:** 8 frames já gerados no `frames.h` anexo (98×107 cada, RGB565 com transparência 0xF81F)
2. **Animações que ficam:** idle, blink, sleep, yawn, alert (frame 7)
3. **Animações que NÃO usar:** waveA (2), waveB (3), love (4) — mantém no array mas não ativa
4. **Borboletas:** MANTÉM (charme ambiente, spawn aleatório)
5. **Zzz quando dormindo:** MANTÉM
6. **Corações flutuantes:** REMOVE (eram pro estado love que não vamos usar)
7. **Respiração quando dormindo:** MANTÉM
8. **Sombra no chão:** MANTÉM
9. **Botões:** APENAS pra acordar quando dormindo. QUALQUER botão (BOOT GPIO 0 ou KEY GPIO 14) = reseta `lastInteraction`
10. **Temperatura:** ADIADA pra fase futura, NÃO implementar agora
11. **Layout overlay:** propor uma versão inicial, vai ser ajustada depois
12. **Sleep timeout:** 30 segundos sem interação → entra em modo sonolento
13. **Alerta:** frame 7 do gato aparece quando API retorna evento ativo. Texto do evento sobreposto.

---

## 📂 Arquivos disponíveis (anexos)

1. `frames.h` — 8 frames de gato, 98×107 cada, ~164KB PROGMEM
2. `butterfly.h` — 2 frames de borboleta, 96×102 cada, ~38KB PROGMEM
3. `preview_all_frames.png` — preview visual dos 8 frames (referência)

⚠️ O `frames.h` tem **constantes nomeadas** que você DEVE usar no código:
```c
#define CAT_FRAME_IDLE   0
#define CAT_FRAME_BLINK  1
#define CAT_FRAME_WAVEA  2  // não vamos ativar
#define CAT_FRAME_WAVEB  3  // não vamos ativar
#define CAT_FRAME_LOVE   4  // não vamos ativar
#define CAT_FRAME_SLEEP  5
#define CAT_FRAME_YAWN   6
#define CAT_FRAME_ALERT  7
```

---

## 🗂️ Plano de execução (7 commits sequenciais)

> **REGRA DE OURO:** cada commit deve compilar limpo e funcionar. Testar antes de prosseguir.

### Commit 1: Copiar sprites
- Criar `firmware/src/sprites/`
- Copiar `frames.h` e `butterfly.h` para essa pasta
- Adicionar `#include "sprites/frames.h"` e `#include "sprites/butterfly.h"` no `render.h`
- `pio run -e t-display-s3` deve compilar sem erro
- **Validação:** Flash usage cresce ~200KB (frames+butterfly), RAM permanece ~50KB
- Commit: `feat(firmware): adiciona sprites de gato (8 frames) e borboleta (2 frames)`

### Commit 2: Sprite buffer (TFT_eSprite) + render do gato idle estático
- Adicionar `TFT_eSprite fb(&tft)` em `render.cpp`
- Em setup, `fb.setColorDepth(16); fb.createSprite(tft.width(), tft.height());`
- Função `drawCatFrame(int ox, int oy, int frameIdx)` que percorre pixel a pixel, ignora `CAT_TRANSPARENT`
- No loop de render principal:
  - `fb.fillSprite(COL_BG)` (cor de fundo bege/clara, ex: 0xCEFB)
  - Desenha chão (igual ao sketch original — `fb.fillRect(0, h-8, w, 8, COL_GROUND)`)
  - Desenha gato frame 0 centralizado: `drawCatFrame((320-98)/2, (170-107)/2+4, CAT_FRAME_IDLE)`
  - `fb.pushSprite(0, 0)`
- **Validação:** Aparece gatinho idle bonito centralizado, sem flicker
- **RAM check:** `createSprite(320, 170)` usa 320*170*2 = ~108KB → uso total deve ficar ~50% (sobra confortável)
- Commit: `feat(firmware): renderiza gato idle bonito com sprite buffer (double-buffer)`

### Commit 3: Blink + sleep + bocejo
- Adicionar timer de blink (a cada 5s, dura 500ms)
- Adicionar `lastInteraction` (millis) e `sleepTimeout = 30000`
- Lógica:
  - Se `now - lastInteraction > 30000` → dormindo (frame 5)
  - Rising edge de "dormindo → acordado" → bocejo por 1500ms (frame 6)
  - Se acordado e não bocejando: alterna idle (frame 0) / blink (frame 1) pelo timer
- Adicionar leitura dos botões BOOT (GPIO 0) e KEY (GPIO 14) com `INPUT_PULLUP`. **QUALQUER** botão pressionado = `lastInteraction = now`
- Respiração quando dormindo: `bob = (int)(sin(now*0.0018f) * SCALE)`, sprite sobe/desce ~1-2px
- **Validação:** Gato pisca de tempos em tempos. Espera 30s sem tocar nada → dorme. Aperta botão → acorda com bocejo
- Commit: `feat(firmware): adiciona blink, sleep timer, bocejo ao acordar e botoes pra acordar`

### Commit 4: Borboletas + Zzz
- Implementar `struct Butterfly` e `struct Zzz` (copiar a estrutura do sketch standalone `gatinho_esp32.ino` mas SEM corações)
- Bitmap do Zzz (9×9) embedded no código:
```c
static const uint8_t Z_BITMAP[9][9] = {
  {1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,1,1,0}, {0,0,0,0,0,1,1,0,0},
  {0,0,0,0,1,1,0,0,0}, {0,0,0,1,1,0,0,0,0},
  {0,0,1,1,0,0,0,0,0}, {1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1},
};
```
- Borboletas: spawn a cada 30-80s aleatoriamente, voam atravessando a tela, asas flap a 140ms
- Borboletas DESENHADAS ANTES do gato (atrás)
- Zzz spawnam a cada 1.4s quando dormindo, sobem com wobble
- **Validação:** Borboletas atravessam a tela ocasionalmente. Quando dormindo, Zzz flutuam saindo da cabeça
- Commit: `feat(firmware): adiciona borboletas no fundo e Zzz quando dormindo`

### Commit 5: Frame 8 (alerta) integrado com eventos da API
- Adicionar estado: se há um evento ativo (do state.h / network.cpp), gato vira frame 7 (ALERT)
- Prioridade no render (do maior pro menor):
  1. ALERT (se evento ativo na próxima 1 min) → frame 7
  2. YAWN (se acabou de acordar) → frame 6
  3. SLEEP (se dormindo) → frame 5
  4. BLINK (se piscando agora) → frame 1
  5. IDLE → frame 0
- Texto do evento aparece sobreposto (vai ser refinado no commit 7)
- **Validação:** Cria evento de teste no frontend (~3min no futuro). No horário, gato muda pra frame 7 e mostra texto
- Commit: `feat(firmware): gato muda pra estado de alerta (frame 7) quando ha evento ativo`

### Commit 6: ~~Temperatura via Open-Meteo~~ — ADIADO

Pular este commit por ora. Próxima sessão.

### Commit 7: Overlay final — hora, data e info no topo
- Barra superior de ~20px de altura com fundo semi-transparente (ou opaco escuro)
- **À esquerda:** hora `HH:MM` em fonte grande (LOAD_FONT4)
- **Centro:** data `Sab 23/Mai` em fonte menor (LOAD_FONT2)
- **À direita:** [vazio por enquanto, vai virar temperatura depois]
- Quando há alerta: barra mostra título do evento + ícone (texto rolando se for longo)
- **Validação:** Vê hora, data legíveis no topo, gato animado embaixo, alertas funcionam
- Commit: `feat(firmware): adiciona overlay de hora/data no topo da tela`

---

## ⚠️ Avisos importantes

### RAM monitoring
Depois de cada commit, rodar `pio run -e t-display-s3` e olhar:
```
RAM:   [==        ]  XX.X% (used Y bytes from 327680 bytes)
Flash: [===       ]  XX.X% (used Y bytes from 3342336 bytes)
```

Limites:
- RAM acima de **60%** = WARNING (pode dar memory crash em runtime com WiFi/HTTP)
- RAM acima de **75%** = STOP e otimizar
- Flash não é problema (sobra ~2.4MB)

Causa principal de RAM alto: `createSprite(320, 170)` = 108KB. Se passar, considerar sprite menor (ex: 320×140 e desenhar resto direto na tela).

### Não quebrar o que funciona
- `network.cpp/h` faz polling de eventos. NÃO MEXER.
- `state.cpp/h` mantém estado global. Adicionar campos novos OK, mas não mexer no que existe.
- `config.h` tem credenciais. NÃO TOCAR.

### Configurações já corretas em `platformio.ini`
- `-DARDUINO_USB_CDC_ON_BOOT=1` ← logs aparecem no monitor
- `-DARDUINO_USB_MODE=1`
- TFT_eSPI inline (parallel 8-bit, todos pinos do T-Display-S3)
- `monitor_speed = 115200`

Se a Fase A funcionou, **NÃO mexer** nessas flags.

### Conhecidos warnings inofensivos
- `DMA is not supported in parallel mode` ← normal pro T-Display-S3
- `TOUCH_CS pin not defined` ← projeto não tem touch, normal

---

## 🐛 Debug check-points (se algo der errado)

### Sintoma: tela preta após upload
- Verificar `LCD_POWER_PIN (15)` e `LCD_BL_PIN (38)` setados HIGH no setup
- Verificar `tft.init(); tft.setRotation(1)` chamados antes de qualquer draw

### Sintoma: gato aparece com cores erradas / pixels embaralhados
- Verificar que `pgm_read_word()` é usado pra ler do array em PROGMEM
- Verificar que cor de transparência `0xF81F` é checada e PULADA (não desenhada)
- Verificar `setColorDepth(16)` no sprite

### Sintoma: flicker / piscamento
- TEM que usar `TFT_eSprite` (double-buffer), não desenhar direto na `tft`
- Verificar que `fb.pushSprite(0, 0)` é chamado UMA vez no fim do loop, não múltiplas

### Sintoma: WiFi para de funcionar
- Verificar que `delay()` longo não trava o loop
- Verificar que `loop()` não está bloqueando >100ms

### Sintoma: heap exhausted / crash
- RAM excedeu. Reduzir sprite buffer ou simplificar.

---

## 📋 Validação final da Fase B

Antes de marcar como completa, conferir:
- [ ] Gato bonito aparece centralizado, sem flicker
- [ ] Gato pisca de tempos em tempos
- [ ] Esperar 30s sem tocar → dorme (frame 5) com Zzz subindo
- [ ] Aperta botão → acorda com bocejo
- [ ] Borboletas atravessam a tela ocasionalmente
- [ ] Quando há evento ativo na API: gato vira frame 7 (alerta) com balão "!"
- [ ] Hora e data aparecem legíveis no topo
- [ ] WiFi/API funcionando (polling não para)
- [ ] RAM uso < 60%
- [ ] Flash uso < 50%
- [ ] Compromisso de teste dispara alerta corretamente

---

## 🎁 Referência: código standalone original

O usuário tem um sketch funcional em `C:\dev\gatinho_esp32\gatinho_esp32.ino` que já implementa toda essa lógica (mas SEM WiFi/API). Usar como **referência de implementação** mas adaptar pro contexto PlatformIO + multi-arquivo.

Trechos importantes a "copiar a lógica" (não literal):
- `drawCatFrame()` — render pixel a pixel com check de transparência
- `struct Heart`, `struct Zzz`, `struct Butterfly` — usar Zzz e Butterfly. Heart NÃO.
- Lógica de timer de blink, sleep timeout, bocejo rising-edge
- Spawn aleatório de borboletas
- Respiração quando dormindo (sin wave no bob vertical)

**ADAPTAÇÕES OBRIGATÓRIAS:**
- Sem botões mapeados pra wave/love (esses estados são mortos)
- Botões BOOT e KEY: ÚNICA função = resetar `lastInteraction`
- Sem corações
- Sem walking mode (gato sempre parado no centro)
- Alerta novo: frame 7, ativado por evento da API
