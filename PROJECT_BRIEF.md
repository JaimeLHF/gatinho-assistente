# Gatinho Pet — Companion Device

Pet virtual num display que mostra hora, temperatura e avisa de compromissos cadastrados via web. Hardware: LilyGo T-Display-S3 (ESP32-S3). Aplicação web para gerenciar os lembretes que aparecem no dispositivo.

## Arquitetura

Monorepo com três pacotes independentes:

```
gatinho-pet/
├── api/         Backend Node.js + Express + Prisma + MySQL
├── web/         Frontend React + Vite + TypeScript
├── firmware/    Código ESP32 em C++ (PlatformIO)
└── README.md
```

Fluxo: o usuário cadastra compromissos pelo frontend → API persiste no MySQL → ESP32 faz polling HTTP a cada 60s consultando próximos eventos → quando bate a hora, o gato na tela entra em modo alerta.

## Stack

**Backend (`api/`)**
- Node.js 20+ com TypeScript
- Express 4
- Prisma como ORM
- MySQL 8
- JWT para autenticação (jsonwebtoken)
- bcrypt para hash de senha
- Zod para validação de entrada
- dotenv para configuração

**Frontend (`web/`)**
- React 18 com TypeScript
- Vite como bundler
- React Router para navegação
- TanStack Query para data fetching
- React Hook Form + Zod para formulários
- Axios para HTTP
- Tailwind CSS para estilos (a confirmar com o dev se preferir outra coisa)

**Firmware (`firmware/`)**
- PlatformIO com framework Arduino
- Target: `esp32-s3-devkitc-1` (mas usando configuração específica do T-Display-S3)
- Bibliotecas: TFT_eSPI, ArduinoJson, HTTPClient, WiFi
- Estrutura modular: separação clara entre render, network, state

## Decisões de produto

- **Multi-usuário com login simples** (poucas pessoas, mas com auth real via JWT).
- **Cada usuário pode ter vários dispositivos**. Cada dispositivo tem um token único que o autoriza a consumir a API.
- **Polling, não push**. O ESP32 consulta a API a cada minuto perguntando "tem evento nos próximos 10 minutos pra esse dispositivo?".
- **Rodando local agora**, deploy depois. Tudo em localhost + MySQL local.
- **TypeScript em tudo** (back e front).

## Modelo de dados (inicial)

```
User
  id, email (unique), passwordHash, name, createdAt

Device
  id, userId (FK), name, token (unique, secreto), lastSeenAt, createdAt

Event
  id, userId (FK), title, description?, startsAt, durationMin?, 
  alertMinutesBefore (default 5), status (pending|done|dismissed),
  createdAt, updatedAt

RefreshToken
  id, userId (FK), tokenHash, expiresAt, revokedAt?, createdAt
```

## Convenções

- **REST** para todas as rotas. Versionamento via prefixo: `/api/v1/...`
- **Respostas JSON** com formato consistente: sucesso retorna o recurso direto; erro retorna `{ error: { code, message } }` com status HTTP apropriado.
- **Auth**: header `Authorization: Bearer <jwt>` para usuários humanos; header `X-Device-Token: <token>` para o ESP32. Middlewares separados.
- **Validação**: todo body de POST/PUT/PATCH passa por schema Zod antes de chegar no controller.
- **Erros**: classe `ApiError` customizada com código + status. Middleware global de error handling.
- **Logs**: pino para logs estruturados.
- **Testes**: vitest no backend e frontend. Foco em testes de integração nas rotas críticas.

## Padrão de pastas (backend)

```
api/
├── src/
│   ├── routes/         Definição de rotas Express
│   ├── controllers/    Handlers (recebem req, chamam service, retornam res)
│   ├── services/       Lógica de negócio (não conhecem Express)
│   ├── middlewares/    auth, errorHandler, validate(schema)
│   ├── schemas/        Schemas Zod compartilhados
│   ├── lib/            prisma client, jwt helpers, logger
│   ├── config/         Env vars com validação Zod
│   └── server.ts       Entry point
├── prisma/
│   └── schema.prisma
└── tests/
```

## Padrão de pastas (frontend)

```
web/
├── src/
│   ├── pages/          Rotas (Login, Dashboard, Events, Devices)
│   ├── components/     Componentes reutilizáveis (UI + features)
│   ├── hooks/          Custom hooks (useAuth, useEvents)
│   ├── api/            Cliente axios + funções por recurso
│   ├── lib/            Utilities, formatters
│   ├── types/          TypeScript types compartilhados
│   └── main.tsx
```

## Endpoints planejados

```
POST   /api/v1/auth/register          email, password, name → { user, accessToken, refreshToken }
POST   /api/v1/auth/login             email, password       → { user, accessToken, refreshToken }
POST   /api/v1/auth/refresh           refreshToken          → { accessToken, refreshToken }
POST   /api/v1/auth/logout            (revoga refresh)      → 204

GET    /api/v1/me                                           → user atual

GET    /api/v1/events                 ?from=&to=            → lista de eventos do user
POST   /api/v1/events                 body                  → cria evento
GET    /api/v1/events/:id                                   → evento específico
PATCH  /api/v1/events/:id             body parcial          → atualiza
DELETE /api/v1/events/:id                                   → remove

GET    /api/v1/devices                                      → lista dispositivos do user
POST   /api/v1/devices                { name }              → cria + retorna token (mostrado UMA vez)
DELETE /api/v1/devices/:id                                  → revoga

GET    /api/v1/device/next-event      header X-Device-Token → próximo evento do user dono do device
                                                              (response inclui currentTime do server pra ESP sincronizar)
```

## Variáveis de ambiente

Backend (`api/.env`):
```
DATABASE_URL=mysql://root:senha@localhost:3306/gatinho_pet
JWT_ACCESS_SECRET=...
JWT_REFRESH_SECRET=...
JWT_ACCESS_EXPIRES_IN=15m
JWT_REFRESH_EXPIRES_IN=30d
PORT=3001
NODE_ENV=development
CORS_ORIGIN=http://localhost:5173
```

Frontend (`web/.env`):
```
VITE_API_URL=http://localhost:3001/api/v1
```

## Plano de implementação (fases)

Cada fase é um prompt separado pro Claude Code:

0. Setup do monorepo (gitignore, README, estrutura)
1. Backend base (Express, Prisma, MySQL, schema inicial, healthcheck)
2. Backend auth (register/login/refresh/logout + middleware JWT)
3. Backend eventos (CRUD)
4. Backend device tokens (criar, revogar, autenticar via header)
5. Frontend base (Vite, React Router, Tailwind, layout)
6. Frontend auth (login/register, persistência, rota protegida)
7. Frontend eventos (lista, criar, editar)
8. Frontend dispositivos (criar token, listar)
9. Firmware ESP32 (PlatformIO setup, Wi-Fi, polling, alerta visual integrado ao gato)
10. Polish (testes, docs, scripts de dev)

## Observações importantes

- Sempre rodar `prisma generate` após alterar `schema.prisma`.
- Antes de criar nova rota, validar se já existe controller/service apropriado.
- Manter handlers thin: validação no middleware, lógica no service, retorno no controller.
- Nunca commitar `.env` (já no gitignore).
- Tokens de dispositivo devem ser longos (32+ bytes random) e hasheados no banco (igual senha) — só retornar plaintext na criação.
- Sempre usar `Math.round`/`toFixed` ao lidar com horários e durações antes de serializar.
