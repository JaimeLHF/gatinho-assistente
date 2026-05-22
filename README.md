# Gatinho Pet — Companion Device

Pet virtual num display LilyGo T-Display-S3 (ESP32-S3) que mostra hora e avisa de compromissos cadastrados via aplicacao web.

> Decisoes de arquitetura, stack e convencoes em [`PROJECT_BRIEF.md`](./PROJECT_BRIEF.md).

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

## Pre-requisitos

- Node.js 20+
- pnpm
- MySQL 8 (ou via Docker: `docker compose up -d mysql`)
- PlatformIO (apenas para o firmware)

## Quick start

```bash
# 1. MySQL
docker compose up -d mysql

# 2. API
cd api
cp .env.example .env
pnpm install
pnpm prisma:generate
pnpm prisma:migrate
cd ..

# 3. Web
cd web
cp .env.example .env
pnpm install
cd ..

# 4. Rodar tudo (API + Web em paralelo)
pnpm dev
```

- API: http://localhost:3001 (healthcheck: `GET /api/v1/health`)
- Web: http://localhost:5173

## Scripts (raiz)

| Comando | Descricao |
|---|---|
| `pnpm dev` | API + Web em paralelo |
| `pnpm build` | Build de producao (API + Web) |
| `pnpm type-check` | Type-check API e Web |
| `pnpm test` | Roda testes de integracao da API |

## Estrutura

```
gatinho-pet/
├── api/         Backend Express + Prisma + MySQL
├── web/         Frontend React + Vite + Tailwind
├── firmware/    ESP32-S3 (PlatformIO + TFT_eSPI)
```

Cada pacote tem seu proprio `README.md` com instrucoes detalhadas.
