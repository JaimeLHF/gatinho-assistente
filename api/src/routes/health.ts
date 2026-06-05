import { Router } from "express";
import { prisma } from "../lib/prisma.js";

const router = Router();

router.get("/health", async (_req, res) => {
  try {
    await prisma.$queryRawUnsafe("SELECT 1");
    res.json({ status: "ok", db: "connected", time: new Date().toISOString() });
  } catch {
    res.status(503).json({ status: "degraded", db: "disconnected", time: new Date().toISOString() });
  }
});

export default router;
