# Gatinho Pet — API

Backend Node.js + Express + Prisma + MySQL.

## Pré-requisitos

- Node.js 20+
- pnpm
- MySQL 8 (ou Docker)

## Subindo o MySQL com Docker

Na raiz do monorepo:

```bash
docker compose up -d mysql
```

Isso cria o banco `gatinho_pet` com senha `senha` na porta 3306. Adminer disponível em `http://localhost:8080`.

## Setup

```bash
cp .env.example .env    # ajuste se necessário
pnpm install
pnpm prisma:generate
pnpm prisma:migrate     # após MySQL rodando
pnpm dev
```

Servidor em `http://localhost:3001`. Healthcheck: `GET /api/v1/health`.
