# Como usar este pacote

Você tem três arquivos:

- **`PROJECT_BRIEF.md`** — o contrato do projeto. Coloque na **raiz** do repositório. É a fonte de verdade que o Claude Code consulta a cada nova sessão.
- **`PROMPT_0_setup.md`** — o primeiro prompt, pronto pra colar.
- **`PROMPTS_GUIDE.md`** (este) — instruções e templates pros próximos prompts.

## Fluxo recomendado

1. Crie a pasta do projeto (`mkdir gatinho-pet && cd gatinho-pet`).
2. Coloque `PROJECT_BRIEF.md` dentro dela.
3. Rode `claude` (ou abra Claude Code apontado pra essa pasta).
4. **Sempre comece a sessão** dizendo `Leia o PROJECT_BRIEF.md antes de qualquer coisa.` — isso garante que ele entra com contexto.
5. Cole o Prompt 0. Espere terminar, revise o resultado, faça commit se gostou.
6. Para cada próxima fase, use o template abaixo.

## Template universal de prompt de fase

```
Vamos para a Fase N: <nome da fase>.

Releia o PROJECT_BRIEF.md se precisar. Sua tarefa:

<lista de itens objetivos, numerados>

Restrições:
- Trabalhe apenas dentro da pasta `<pasta>/` (api, web ou firmware).
- Não altere arquivos de outras fases sem me consultar.
- Siga os padrões definidos no brief (estrutura de pastas, validação Zod, etc).
- Antes de instalar libs novas, me mostre a lista e justifique.

Ao terminar:
- Mostre os arquivos criados/alterados.
- Rode o que for testável (build, lint, type-check, healthcheck) e mostre a saída.
- Faça commit com mensagem `feat(<scope>): <descrição>`.

Pare e espere meu OK antes de seguir pra próxima fase.
```

## Prompts prontos para cada fase

### Fase 1 — Backend base

```
Vamos para a Fase 1: Backend base.

Releia o PROJECT_BRIEF.md. Sua tarefa, tudo dentro de `api/`:

1. Inicialize um projeto Node.js + TypeScript com pnpm.
2. Instale e configure: express, @types/express, typescript, ts-node-dev, tsx, pino, pino-pretty, zod, dotenv, cors, @types/cors.
3. Instale e configure Prisma: prisma, @prisma/client. Inicialize com `prisma init --datasource-provider mysql`.
4. Crie `schema.prisma` com os modelos User, Device, Event, RefreshToken conforme PROJECT_BRIEF.md. NÃO rode `migrate` ainda — apenas formate e valide o schema.
5. Crie a estrutura de pastas: src/{routes,controllers,services,middlewares,schemas,lib,config}, tests/.
6. Implemente:
   - src/config/env.ts: validação de env vars com Zod e export tipado.
   - src/lib/prisma.ts: singleton do PrismaClient.
   - src/lib/logger.ts: instância de pino configurada.
   - src/middlewares/errorHandler.ts: middleware global de erro + classe ApiError.
   - src/middlewares/validate.ts: factory `validate(schema)` que valida body/query/params.
   - src/server.ts: bootstrap do Express com cors, json, logger, errorHandler. Rota GET /api/v1/health retornando { status: "ok", time }.
7. Configure tsconfig.json estrito (strict: true, noUncheckedIndexedAccess: true) e crie scripts: dev, build, start, type-check, prisma:generate, prisma:studio.
8. Crie .env.example com as vars do brief.
9. Adicione um README curto em api/ explicando como subir o MySQL local (pode usar docker compose — sugira um docker-compose.yml na raiz se achar útil).

Restrições e finalização: igual ao template.
```

### Fase 2 — Backend auth

```
Vamos para a Fase 2: Autenticação.

Sua tarefa, dentro de `api/`:

1. Rode `prisma migrate dev --name init` agora que a Fase 1 está estável (confirme o DATABASE_URL antes).
2. Instale: bcrypt, @types/bcrypt, jsonwebtoken, @types/jsonwebtoken.
3. Crie src/lib/jwt.ts com helpers signAccess, signRefresh, verifyAccess, verifyRefresh.
4. Crie src/services/authService.ts com: register, login, refresh, logout. Refresh tokens devem ser hasheados e persistidos na tabela RefreshToken (rotacionar a cada refresh).
5. Crie src/schemas/auth.ts com schemas Zod: registerSchema, loginSchema, refreshSchema.
6. Crie src/controllers/authController.ts e src/routes/auth.ts montando POST /api/v1/auth/{register,login,refresh,logout}.
7. Crie src/middlewares/authenticate.ts que valida Bearer token e injeta `req.user`.
8. Crie src/routes/me.ts com GET /api/v1/me (protegido).
9. Adicione testes de integração (vitest + supertest) cobrindo: register feliz, login com senha errada, refresh válido, /me sem token retorna 401.

Restrições e finalização: igual ao template. Inclua os comandos pra rodar os testes.
```

### Fase 3 — Backend eventos

```
Vamos para a Fase 3: CRUD de eventos.

Dentro de `api/`:

1. Crie src/schemas/event.ts com createEventSchema, updateEventSchema, listEventsQuerySchema (com from/to opcionais como ISO datetime).
2. Crie src/services/eventService.ts com list, create, getById, update, remove. Todas as queries DEVEM filtrar por userId — nunca permita acesso cross-user.
3. Crie controller + rotas em /api/v1/events (todas protegidas pelo authenticate).
4. Adicione testes cobrindo: criar evento, listar eventos do user (não vê de outro user), update de evento de outro user retorna 404, delete.

Restrições e finalização: igual ao template.
```

### Fase 4 — Backend device tokens

```
Vamos para a Fase 4: Device tokens para o ESP32.

Dentro de `api/`:

1. Crie src/services/deviceService.ts:
   - create(userId, name): gera token de 32 bytes random (crypto.randomBytes), retorna { device, plainToken } APENAS uma vez. Persiste tokenHash (sha256 ou bcrypt).
   - list(userId)
   - revoke(userId, deviceId)
   - authenticateByToken(plainToken): retorna { device, user } ou null. Atualiza lastSeenAt.
2. Crie src/middlewares/authenticateDevice.ts: lê header X-Device-Token, valida, injeta req.device e req.user.
3. Crie rotas POST /api/v1/devices, GET /api/v1/devices, DELETE /api/v1/devices/:id (protegidas por authenticate normal de usuário).
4. Crie src/services/deviceQueryService.ts com getNextEventForUser(userId, withinMinutes = 10) — retorna o próximo evento pending dentro da janela ou null.
5. Crie rota GET /api/v1/device/next-event (protegida por authenticateDevice). Response deve incluir { event: ... | null, serverTime: ISO string }.
6. Testes: criar device retorna plainToken só uma vez; chamar next-event com token inválido retorna 401; com token válido retorna evento correto ou null.

Restrições e finalização: igual ao template.
```

### Fase 5 a 8 — Frontend

A partir daqui peça uma fase por vez. Sugestão de divisão:

- **Fase 5**: setup Vite + React + TS + Tailwind + React Router + TanStack Query + Axios + interceptor de auth + layout base com nav.
- **Fase 6**: páginas Login e Register, persistência de tokens (localStorage), AuthContext/useAuth, rota protegida, refresh automático no interceptor.
- **Fase 7**: página Events com lista, criar (modal/drawer), editar inline, deletar com confirmação. Use React Hook Form + Zod.
- **Fase 8**: página Devices com lista, botão "novo dispositivo" que mostra o token UMA vez com botão de copiar.

### Fase 9 — Firmware ESP32

```
Vamos para a Fase 9: Firmware no ESP32.

Migre o código atual do Arduino IDE pra PlatformIO dentro de `firmware/`.

1. Crie platformio.ini configurado pro LilyGo T-Display-S3 (board lilygo-t-display-s3 ou esp32-s3-devkitc-1 com flags adequadas). Configure TFT_eSPI via build_flags (Setup206) em vez de editar arquivos da lib.
2. Estrutura:
   - src/main.cpp: loop principal e setup.
   - src/render/ (cat.cpp/.h, screens.cpp/.h): tudo de desenho na tela.
   - src/network/ (wifi.cpp/.h, api.cpp/.h): conexão Wi-Fi e chamadas à API.
   - src/state/ (state.cpp/.h): estado global (modo atual, evento ativo).
   - include/config.h.example: WIFI_SSID, WIFI_PASS, API_URL, DEVICE_TOKEN. include/config.h fica no gitignore.
3. Implemente:
   - Conexão Wi-Fi com retry.
   - Sincronização NTP (fuso BRT3).
   - Cliente HTTP que faz GET na rota /device/next-event a cada 60s, com header X-Device-Token.
   - Parsing JSON com ArduinoJson.
   - Máquina de estados: IDLE (gato passeando + hora/temp), ALERT (evento próximo: gato no centro, borda piscando, título do evento), FEED/HAPPY (botões 1/2 como já tínhamos).
   - Botões: btn1 dispensa alerta, btn2 dá soneca (5min).
4. Adicione README em firmware/ explicando como copiar o config.h.example, configurar credenciais, e fazer upload.

Use o código que já desenvolvemos no chat como base — me peça se quiser que eu cole o sketch atual.

Restrições e finalização: igual ao template.
```

### Fase 10 — Polish

```
Vamos para a Fase 10: Polish final.

1. Verifique cobertura mínima dos testes críticos. Adicione o que faltar.
2. Adicione um docker-compose.yml na raiz com MySQL 8 e adminer (opcional).
3. Atualize o README raiz com instruções end-to-end: clonar, subir DB, rodar migrations, seed (se houver), rodar api+web, configurar firmware.
4. Crie script `pnpm dev` na raiz que sobe api e web em paralelo (use concurrently ou turbo).
5. Adicione lint (eslint) e formatter (prettier) compartilhados, com configs no monorepo.
6. Rode lint, type-check e testes em tudo. Conserte o que aparecer.
```

## Dicas finais

- **Use `@` para anexar arquivos**: no Claude Code, digite `@PROJECT_BRIEF.md` pra incluir o arquivo no contexto sem colar conteúdo.
- **Quando ele errar, seja específico**: em vez de "tá errado, conserta", diga "o validate middleware tá retornando 500 em vez de 422 quando o Zod falha — me mostra o arquivo e ajusta".
- **Commits frequentes**: peça commit ao fim de cada fase. Se ele errar, você reverta. Não deixe acumular.
- **Não pule fases**: se ele propor "já faço a 3 junto", recuse. Uma por vez = controle.
- **Quando precisar ajustar o brief**: edite o `PROJECT_BRIEF.md` diretamente, peça pra ele reler. Ele é vivo.

Bom projeto!
