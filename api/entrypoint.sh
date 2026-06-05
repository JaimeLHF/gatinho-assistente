#!/bin/sh
set -e

echo "[entrypoint] Running Prisma migrations..."
npx prisma migrate deploy --schema=api/prisma/schema.prisma

echo "[entrypoint] Starting API server..."
exec node api/dist/server.js
