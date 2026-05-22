import type { Request, Response, NextFunction } from "express";
import type { ZodType } from "zod/v4";
import { ApiError } from "./errorHandler.js";

interface ValidateSchemas {
  body?: ZodType;
  query?: ZodType;
  params?: ZodType;
}

export function validate(schemas: ValidateSchemas) {
  return (req: Request, _res: Response, next: NextFunction): void => {
    if (schemas.body) {
      const result = schemas.body.safeParse(req.body);
      if (!result.success) {
        throw new ApiError(422, "VALIDATION_ERROR", result.error.message);
      }
      req.body = result.data;
    }

    if (schemas.query) {
      const result = schemas.query.safeParse(req.query);
      if (!result.success) {
        throw new ApiError(422, "VALIDATION_ERROR", result.error.message);
      }
      Object.assign(req.query, result.data);
    }

    if (schemas.params) {
      const result = schemas.params.safeParse(req.params);
      if (!result.success) {
        throw new ApiError(422, "VALIDATION_ERROR", result.error.message);
      }
      Object.assign(req.params, result.data);
    }

    next();
  };
}
