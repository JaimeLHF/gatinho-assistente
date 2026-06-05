FROM node:22-slim AS base
RUN corepack enable && corepack prepare pnpm@latest --activate
WORKDIR /app

# ── deps ──
FROM base AS deps
COPY package.json pnpm-lock.yaml pnpm-workspace.yaml ./
COPY web/package.json web/
RUN pnpm install --frozen-lockfile --filter web

# ── build ──
FROM deps AS build
ARG VITE_API_URL=/api/v1
ENV VITE_API_URL=$VITE_API_URL
COPY web/ web/
RUN pnpm --filter web build

# ── production ──
FROM nginx:alpine AS production
COPY --from=build /app/web/dist /usr/share/nginx/html
COPY web/nginx.conf /etc/nginx/conf.d/default.conf
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
