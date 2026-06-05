import { prisma } from "../lib/prisma.js";
import { ApiError } from "../middlewares/errorHandler.js";
import path from "node:path";
import fs from "node:fs";

const UPLOAD_DIR = path.resolve(import.meta.dirname, "../../uploads");

export async function upload(userId: string, version: string, file: Express.Multer.File) {
  // Check duplicate version
  const existing = await prisma.firmware.findUnique({ where: { version } });
  if (existing) {
    throw new ApiError(409, "VERSION_EXISTS", `Firmware version ${version} already exists`);
  }

  // Ensure upload dir exists
  if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });

  // Save file as version.bin
  const filename = `${version}.bin`;
  const dest = path.join(UPLOAD_DIR, filename);
  fs.writeFileSync(dest, file.buffer);

  return prisma.firmware.create({
    data: { version, filename, size: file.size, userId },
    select: { id: true, version: true, size: true, createdAt: true },
  });
}

export async function latest() {
  return prisma.firmware.findFirst({
    orderBy: { createdAt: "desc" },
    select: { id: true, version: true, size: true, createdAt: true },
  });
}

export async function list() {
  return prisma.firmware.findMany({
    orderBy: { createdAt: "desc" },
    select: { id: true, version: true, size: true, createdAt: true },
    take: 20,
  });
}

export async function getFilename(version: string): Promise<string> {
  const fw = await prisma.firmware.findUnique({ where: { version } });
  if (!fw) throw new ApiError(404, "FIRMWARE_NOT_FOUND", "Firmware not found");
  return fw.filename;
}

export async function getFilePath(version: string): Promise<string> {
  const fw = await prisma.firmware.findUnique({ where: { version } });
  if (!fw) throw new ApiError(404, "FIRMWARE_NOT_FOUND", "Firmware not found");

  const filePath = path.join(UPLOAD_DIR, fw.filename);
  if (!fs.existsSync(filePath)) {
    throw new ApiError(404, "FIRMWARE_FILE_MISSING", "Firmware binary not found on disk");
  }

  return filePath;
}

export async function remove(userId: string, id: string) {
  const fw = await prisma.firmware.findUnique({ where: { id } });
  if (!fw) throw new ApiError(404, "FIRMWARE_NOT_FOUND", "Firmware not found");

  // Delete file
  const filePath = path.join(UPLOAD_DIR, fw.filename);
  if (fs.existsSync(filePath)) fs.unlinkSync(filePath);

  await prisma.firmware.delete({ where: { id } });
}
