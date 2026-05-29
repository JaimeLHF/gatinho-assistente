import { randomBytes, createHash } from "node:crypto";
import { prisma } from "../lib/prisma.js";
import { ApiError } from "../middlewares/errorHandler.js";
import type { CreateDeviceInput, UpdateDeviceInput, CustomizationInput } from "../schemas/device.js";

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

export async function getCustomization(userId: string, deviceId: string) {
  const device = await prisma.device.findUnique({ where: { id: deviceId } });

  if (!device || device.userId !== userId) {
    throw new ApiError(404, "DEVICE_NOT_FOUND", "Device not found");
  }

  return prisma.deviceCustomization.findUnique({ where: { deviceId } });
}

export async function upsertCustomization(userId: string, deviceId: string, data: CustomizationInput) {
  const device = await prisma.device.findUnique({ where: { id: deviceId } });

  if (!device || device.userId !== userId) {
    throw new ApiError(404, "DEVICE_NOT_FOUND", "Device not found");
  }

  return prisma.deviceCustomization.upsert({
    where: { deviceId },
    create: { deviceId, ...data },
    update: data,
  });
}

export async function deleteCustomization(userId: string, deviceId: string) {
  const device = await prisma.device.findUnique({ where: { id: deviceId } });

  if (!device || device.userId !== userId) {
    throw new ApiError(404, "DEVICE_NOT_FOUND", "Device not found");
  }

  await prisma.deviceCustomization.deleteMany({ where: { deviceId } });
}

export async function nextEvent(deviceId: string, userId: string) {
  const now = new Date();

  const [event, customization] = await Promise.all([
    prisma.event.findFirst({
      where: {
        userId,
        status: "PENDING",
        startsAt: { gte: now },
      },
      orderBy: { startsAt: "asc" },
    }),
    prisma.deviceCustomization.findUnique({
      where: { deviceId },
      select: { body: true, stripes: true, belly: true, outline: true, eyes: true, nose: true },
    }),
  ]);

  return { event, customization };
}
