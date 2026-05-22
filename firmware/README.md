# Gatinho Pet — Firmware

Firmware para LilyGo T-Display-S3 (ESP32-S3) com PlatformIO.

Mostra hora sincronizada do servidor, um gato animado e alertas de eventos cadastrados via web.

## Pre-requisitos

- [PlatformIO CLI](https://platformio.org/install/cli) ou extensao VS Code
- Cabo USB-C
- LilyGo T-Display-S3

## Setup

```bash
cp src/config.example.h src/config.h
```

Edite `src/config.h` com:
- WiFi SSID e senha
- URL da API (IP do servidor na rede local)
- Device token (criado em `POST /api/v1/devices` via web ou curl)

## Build e upload

```bash
pio run                    # compilar
pio run --target upload    # compilar e gravar no ESP32
pio device monitor         # monitor serial (115200 baud)
```

## Estrutura

```
src/
  main.cpp            Entry point (setup + loop)
  config.h            Credenciais WiFi e API (gitignored)
  config.example.h    Template de configuracao
  network.h/cpp       Conexao WiFi + polling HTTP
  state.h/cpp         Maquina de estados + sync de relogio + dados do evento
  render.h/cpp        Rendering TFT (relogio, gato, alertas)
```

## Fluxo

1. Boot -> conecta WiFi
2. Poll `GET /device/next-event` a cada 60s com `X-Device-Token`
3. Sync relogio do `currentTime` da resposta
4. Sem evento -> gato relaxado
5. Evento dentro do alerta -> gato arregalado + titulo + countdown
