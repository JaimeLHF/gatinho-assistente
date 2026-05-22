import type { Request, Response, NextFunction } from "express";
import { findByToken } from "../services/deviceService.js";
import { ApiError } from "./errorHandler.js";

declare module "express-serve-static-core" {
  interface Request {
    device?: {
      id: string;
      userId: string;
      name: string;
    };
  }
}

export async function authenticateDevice(
  req: Request,
  _res: Response,
  next: NextFunction,
): Promise<void> {
  const token = req.headers["x-device-token"];
  if (typeof token !== "string" || !token) {
    throw new ApiError(401, "MISSING_DEVICE_TOKEN", "Missing X-Device-Token header");
  }

  const device = await findByToken(token);
  req.device = { id: device.id, userId: device.userId, name: device.name };
  next();
}
