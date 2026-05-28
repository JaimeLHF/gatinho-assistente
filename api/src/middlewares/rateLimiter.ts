import rateLimit, { type Options } from "express-rate-limit";

const ipKeyGenerator: Options["keyGenerator"] = (req, _res) => req.ip ?? "unknown";

/** Strict limiter for auth endpoints (login, register, refresh). */
export const authLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  limit: 20,
  standardHeaders: "draft-7",
  legacyHeaders: false,
  message: {
    error: {
      code: "TOO_MANY_REQUESTS",
      message: "Too many requests, please try again later",
    },
  },
});

/** General limiter for authenticated API routes. */
export const apiLimiter = rateLimit({
  windowMs: 15 * 60 * 1000,
  limit: 200,
  standardHeaders: "draft-7",
  legacyHeaders: false,
  message: {
    error: {
      code: "TOO_MANY_REQUESTS",
      message: "Too many requests, please try again later",
    },
  },
});

/** Lenient limiter for device polling (one device polls every 60s). */
export const devicePollLimiter = rateLimit({
  windowMs: 15 * 60 * 1000,
  limit: 30,
  standardHeaders: "draft-7",
  legacyHeaders: false,
  keyGenerator: (req, res) =>
    (req.headers["x-device-token"] as string) ?? ipKeyGenerator(req, res),
  message: {
    error: {
      code: "TOO_MANY_REQUESTS",
      message: "Too many requests, please try again later",
    },
  },
});
