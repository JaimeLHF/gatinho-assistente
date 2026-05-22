# Gatinho Pet — Companion Device

Pet virtual num display LilyGo T-Display-S3 (ESP32-S3) que mostra hora, temperatura e avisa de compromissos cadastrados via aplicação web.

> Decisões de arquitetura, stack e convenções estão documentadas em [`PROJECT_BRIEF.md`](./PROJECT_BRIEF.md).

## Arquitetura

```
┌──────────┐       HTTP/REST        ┌──────────┐       MySQL       ┌───────┐
│  Web App │ ────────────────────▶  │   API    │ ───────────────▶  │  DB   │
│ (React)  │  ◀────────────────────  │ (Express)│  ◀───────────────  │       │
└──────────┘                        └──────────┘                   └───────┘
                                         ▲
                                         │ polling HTTP (60s)
                                         │ X-Device-Token
                                    ┌──────────┐
                                    │  ESP32   │
                                    │ firmware │
                                    └──────────┘
```

## Como rodar localmente

### API

```bash
cd api
pnpm install
pnpm dev
# http://localhost:3001
```

### Web

```bash
cd web
pnpm install
pnpm dev
# http://localhost:5173
```

### Firmware

```bash
cd firmware
# Abrir com PlatformIO no VS Code
# Build: pio run
# Upload: pio run --target upload
```

### Tudo junto (api + web)

```bash
pnpm dev
```
