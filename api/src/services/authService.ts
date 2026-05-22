import { createHash } from "node:crypto";
import bcrypt from "bcrypt";
import jwt from "jsonwebtoken";
import { prisma } from "../lib/prisma.js";
import { signAccess, signRefresh, verifyRefresh } from "../lib/jwt.js";
import { ApiError } from "../middlewares/errorHandler.js";
import type { RegisterInput, LoginInput } from "../schemas/auth.js";

const SALT_ROUNDS = 10;

function hashToken(token: string): string {
  return createHash("sha256").update(token).digest("hex");
}

function getExpiresAt(token: string): Date {
  const decoded = jwt.decode(token) as jwt.JwtPayload;
  return new Date(decoded.exp! * 1000);
}

function userDto(user: { id: string; email: string; name: string }) {
  return { id: user.id, email: user.email, name: user.name };
}

export async function register(input: RegisterInput) {
  const existing = await prisma.user.findUnique({
    where: { email: input.email },
  });
  if (existing) {
    throw new ApiError(409, "EMAIL_TAKEN", "Email already registered");
  }

  const passwordHash = await bcrypt.hash(input.password, SALT_ROUNDS);
  const user = await prisma.user.create({
    data: { email: input.email, passwordHash, name: input.name },
  });

  const accessToken = signAccess({ sub: user.id, email: user.email });
  const refreshToken = signRefresh({ sub: user.id, email: user.email });

  await prisma.refreshToken.create({
    data: {
      userId: user.id,
      tokenHash: hashToken(refreshToken),
      expiresAt: getExpiresAt(refreshToken),
    },
  });

  return { user: userDto(user), accessToken, refreshToken };
}

export async function login(input: LoginInput) {
  const user = await prisma.user.findUnique({
    where: { email: input.email },
  });
  if (!user) {
    throw new ApiError(401, "INVALID_CREDENTIALS", "Invalid email or password");
  }

  const valid = await bcrypt.compare(input.password, user.passwordHash);
  if (!valid) {
    throw new ApiError(401, "INVALID_CREDENTIALS", "Invalid email or password");
  }

  const accessToken = signAccess({ sub: user.id, email: user.email });
  const refreshToken = signRefresh({ sub: user.id, email: user.email });

  await prisma.refreshToken.create({
    data: {
      userId: user.id,
      tokenHash: hashToken(refreshToken),
      expiresAt: getExpiresAt(refreshToken),
    },
  });

  return { user: userDto(user), accessToken, refreshToken };
}

export async function refresh(oldToken: string) {
  let payload;
  try {
    payload = verifyRefresh(oldToken);
  } catch {
    throw new ApiError(401, "INVALID_TOKEN", "Refresh token is invalid or expired");
  }

  const tokenHash = hashToken(oldToken);
  const stored = await prisma.refreshToken.findUnique({ where: { tokenHash } });

  if (!stored || stored.revokedAt) {
    throw new ApiError(401, "INVALID_TOKEN", "Refresh token is invalid or revoked");
  }

  await prisma.refreshToken.update({
    where: { id: stored.id },
    data: { revokedAt: new Date() },
  });

  const accessToken = signAccess({ sub: payload.sub, email: payload.email });
  const refreshToken = signRefresh({ sub: payload.sub, email: payload.email });

  await prisma.refreshToken.create({
    data: {
      userId: payload.sub,
      tokenHash: hashToken(refreshToken),
      expiresAt: getExpiresAt(refreshToken),
    },
  });

  return { accessToken, refreshToken };
}

export async function logout(token: string) {
  const tokenHash = hashToken(token);
  const stored = await prisma.refreshToken.findUnique({ where: { tokenHash } });

  if (stored && !stored.revokedAt) {
    await prisma.refreshToken.update({
      where: { id: stored.id },
      data: { revokedAt: new Date() },
    });
  }
}
