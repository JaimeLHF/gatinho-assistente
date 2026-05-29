import express from "express";
import cors from "cors";
import { env } from "./config/env.js";
import { errorHandler } from "./middlewares/errorHandler.js";
import { authLimiter, apiLimiter, devicePollLimiter } from "./middlewares/rateLimiter.js";
import healthRouter from "./routes/health.js";
import authRouter from "./routes/auth.js";
import meRouter from "./routes/me.js";
import eventsRouter from "./routes/events.js";
import devicesRouter from "./routes/devices.js";
import deviceRouter from "./routes/device.js";
import firmwareRouter from "./routes/firmware.js";

const app = express();

app.use(cors({ origin: env.CORS_ORIGIN }));
app.use(express.json());

app.use("/api/v1", healthRouter);
app.use("/api/v1/auth", authLimiter);
app.use("/api/v1", authRouter);
app.use("/api/v1/device", devicePollLimiter);
app.use("/api/v1", deviceRouter);
app.use("/api/v1", apiLimiter);
app.use("/api/v1", meRouter);
app.use("/api/v1", eventsRouter);
app.use("/api/v1", devicesRouter);
app.use("/api/v1", firmwareRouter);

app.use(errorHandler);

export default app;
