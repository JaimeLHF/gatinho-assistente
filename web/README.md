# Gatinho Pet — Web

Frontend React + Vite + TypeScript + Tailwind CSS v4.

## Pre-requisitos

- Node.js 20+
- pnpm

## Setup

```bash
cp .env.example .env    # ajuste a URL da API se necessario
pnpm install
pnpm dev
```

Dev server em `http://localhost:5173`.

## Scripts

| Comando | Descricao |
|---|---|
| `pnpm dev` | Inicia o dev server (Vite) |
| `pnpm build` | Type-check + build de producao |
| `pnpm preview` | Serve o build de producao localmente |
| `pnpm type-check` | Verifica tipos sem emitir |

## Estrutura

```
src/
  api/          Cliente Axios + funcoes por recurso
  components/   Componentes reutilizaveis (Layout, EventForm, ProtectedRoute)
  hooks/        Custom hooks (useAuth)
  pages/        Paginas (Login, Register, Dashboard, Events, Devices)
  lib/          QueryClient
  types/        Tipos TypeScript compartilhados
  router.tsx    Definicao de rotas
  main.tsx      Entry point
```
