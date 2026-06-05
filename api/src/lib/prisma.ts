import { PrismaMariaDb } from "@prisma/adapter-mariadb";
import { PrismaClient } from "../generated/prisma/client.js";
import { env } from "../config/env.js";

function buildAdapter() {
  if (env.NODE_ENV !== "production") {
    return new PrismaMariaDb(env.DATABASE_URL);
  }

  const url = new URL(env.DATABASE_URL);
  return new PrismaMariaDb({
    host: url.hostname,
    port: Number(url.port) || 3306,
    user: url.username,
    password: decodeURIComponent(url.password),
    database: url.pathname.slice(1),
    connectionLimit: 10,
    acquireTimeout: 10000,
    idleTimeout: 60000,
  });
}

export const prisma = new PrismaClient({ adapter: buildAdapter() });
