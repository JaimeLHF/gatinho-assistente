import "dotenv/config";
import { z } from "zod/v4";

const envSchema = z.object({
  DATABASE_URL: z.string(),
  JWT_ACCESS_SECRET: z.string(),
  JWT_REFRESH_SECRET: z.string(),
  JWT_ACCESS_EXPIRES_IN: z.string().default("15m"),
  JWT_REFRESH_EXPIRES_IN: z.string().default("30d"),
  PORT: z.coerce.number().default(3001),
  NODE_ENV: z.enum(["development", "production", "test"]).default("development"),
  CORS_ORIGIN: z.string().default("http://localhost:5173"),
  ADMIN_EMAIL: z.string().optional(),
});

const parsed = envSchema.safeParse(process.env);

if (!parsed.success) {
  console.error("❌ Invalid environment variables:");
  console.error(JSON.stringify(parsed.error.format(), null, 2));
  process.exit(1);
}

const UNSAFE_DEFAULTS = ["change-me-access-secret", "change-me-refresh-secret"];

if (
  parsed.data.NODE_ENV === "production" &&
  (UNSAFE_DEFAULTS.includes(parsed.data.JWT_ACCESS_SECRET) ||
    UNSAFE_DEFAULTS.includes(parsed.data.JWT_REFRESH_SECRET))
) {
  console.error(
    "❌ Cannot start in production with default JWT secrets. Set real values in .env.production",
  );
  process.exit(1);
}

export const env = parsed.data;
