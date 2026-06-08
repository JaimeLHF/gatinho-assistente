FROM node:22-slim AS base
RUN corepack enable && corepack prepare pnpm@latest --activate
WORKDIR /app

# ── deps ──
FROM base AS deps
COPY package.json pnpm-lock.yaml pnpm-workspace.yaml ./
COPY api/package.json api/
RUN pnpm install --frozen-lockfile --filter api

# ── build ──
FROM deps AS build
COPY api/ api/
RUN pnpm --filter api prisma:generate
RUN pnpm --filter api build

# ── production ──
FROM base AS production
ENV NODE_ENV=production
COPY package.json pnpm-lock.yaml pnpm-workspace.yaml ./
COPY api/package.json api/
RUN pnpm install --frozen-lockfile --filter api --prod
COPY --from=build /app/api/dist api/dist
COPY --from=build /app/api/src/generated api/src/generated
COPY --from=build /app/api/prisma api/prisma
COPY api/prisma.config.ts api/prisma.config.ts
COPY api/entrypoint.sh api/entrypoint.sh
RUN chmod +x api/entrypoint.sh
EXPOSE 3001
ENTRYPOINT ["api/entrypoint.sh"]
