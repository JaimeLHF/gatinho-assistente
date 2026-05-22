import type { Request, Response, NextFunction } from "express";
import { verifyAccess } from "../lib/jwt.js";
import { ApiError } from "./errorHandler.js";

declare module "express-serve-static-core" {
  interface Request {
    user?: {
      id: string;
      email: string;
    };
  }
}

export function authenticate(req: Request, _res: Response, next: NextFunction): void {
  const header = req.headers.authorization;
  if (!header?.startsWith("Bearer ")) {
    throw new ApiError(401, "UNAUTHORIZED", "Missing or invalid authorization header");
  }

  try {
    const payload = verifyAccess(header.slice(7));
    req.user = { id: payload.sub, email: payload.email };
    next();
  } catch {
    throw new ApiError(401, "UNAUTHORIZED", "Invalid or expired token");
  }
}
