import { randomBytes, createHash } from "node:crypto";
import { prisma } from "../lib/prisma.js";
import { ApiError } from "../middlewares/errorHandler.js";
import type { CreateDeviceInput, UpdateDeviceInput } from "../schemas/device.js";

function hashToken(token: string): string {
  return createHash("sha256").update(token).digest("hex");
}

export async function list(userId: string) {
  return prisma.device.findMany({
    where: { userId },
    select: {
      id: true,
      name: true,
      lastSeenAt: true,
      createdAt: true,
    },
    orderBy: { createdAt: "desc" },
  });
}

export async function create(userId: string, data: CreateDeviceInput) {
  const plainToken = randomBytes(32).toString("hex");
  const tokenHash = hashToken(plainToken);

  const device = await prisma.device.create({
    data: {
      userId,
      name: data.name,
      tokenHash,
    },
    select: {
      id: true,
      name: true,
      createdAt: true,
    },
  });

  return { ...device, token: plainToken };
}

export async function update(userId: string, deviceId: string, data: UpdateDeviceInput) {
  const device = await prisma.device.findUnique({ where: { id: deviceId } });

  if (!device || device.userId !== userId) {
    throw new ApiError(404, "DEVICE_NOT_FOUND", "Device not found");
  }

  return prisma.device.update({
    where: { id: deviceId },
    data,
    select: { id: true, name: true, lastSeenAt: true, createdAt: true },
  });
}

export async function remove(userId: string, deviceId: string) {
  const device = await prisma.device.findUnique({ where: { id: deviceId } });

  if (!device || device.userId !== userId) {
    throw new ApiError(404, "DEVICE_NOT_FOUND", "Device not found");
  }

  await prisma.device.delete({ where: { id: deviceId } });
}

export async function findByToken(plainToken: string) {
  const tokenHash = hashToken(plainToken);
  const device = await prisma.device.findUnique({ where: { tokenHash } });

  if (!device) {
    throw new ApiError(401, "INVALID_DEVICE_TOKEN", "Invalid device token");
  }

  await prisma.device.update({
    where: { id: device.id },
    data: { lastSeenAt: new Date() },
  });

  return device;
}

export async function nextEvent(userId: string) {
  const now = new Date();

  return prisma.event.findFirst({
    where: {
      userId,
      status: "PENDING",
      startsAt: { gte: now },
    },
    orderBy: { startsAt: "asc" },
  });
}
