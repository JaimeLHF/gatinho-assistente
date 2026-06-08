import type { Request, Response, NextFunction } from "express";
import { ApiError } from "./errorHandler.js";

export function authorizeAdmin(req: Request, _res: Response, next: NextFunction): void {
  if (req.user?.role !== "ADMIN") {
    throw new ApiError(403, "FORBIDDEN", "Admin access required");
  }
  next();
}
