import { Router } from "express";
import type { Request, Response } from "express";
import { authenticate } from "../middlewares/authenticate.js";
import { prisma } from "../lib/prisma.js";
import { ApiError } from "../middlewares/errorHandler.js";

const router = Router();

router.get("/me", authenticate, async (req: Request, res: Response) => {
  const user = await prisma.user.findUnique({
    where: { id: req.user!.id },
    select: { id: true, email: true, name: true, createdAt: true },
  });
  if (!user) {
    throw new ApiError(404, "USER_NOT_FOUND", "User not found");
  }
  res.json(user);
});

export default router;
