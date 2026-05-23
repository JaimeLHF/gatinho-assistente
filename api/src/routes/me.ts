import { Router } from "express";
import type { Request, Response } from "express";
import bcrypt from "bcrypt";
import { authenticate } from "../middlewares/authenticate.js";
import { validate } from "../middlewares/validate.js";
import { updateProfileSchema, changePasswordSchema } from "../schemas/me.js";
import { prisma } from "../lib/prisma.js";
import { ApiError } from "../middlewares/errorHandler.js";

const SALT_ROUNDS = 10;
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

router.patch(
  "/me",
  authenticate,
  validate({ body: updateProfileSchema }),
  async (req: Request, res: Response) => {
    if (req.body.email) {
      const existing = await prisma.user.findUnique({
        where: { email: req.body.email },
      });
      if (existing && existing.id !== req.user!.id) {
        throw new ApiError(409, "EMAIL_TAKEN", "Email already in use");
      }
    }

    const user = await prisma.user.update({
      where: { id: req.user!.id },
      data: req.body,
      select: { id: true, email: true, name: true, createdAt: true },
    });
    res.json(user);
  },
);

router.post(
  "/me/change-password",
  authenticate,
  validate({ body: changePasswordSchema }),
  async (req: Request, res: Response) => {
    const user = await prisma.user.findUnique({
      where: { id: req.user!.id },
    });
    if (!user) {
      throw new ApiError(404, "USER_NOT_FOUND", "User not found");
    }

    const valid = await bcrypt.compare(req.body.currentPassword, user.passwordHash);
    if (!valid) {
      throw new ApiError(401, "INVALID_PASSWORD", "Current password is incorrect");
    }

    const passwordHash = await bcrypt.hash(req.body.newPassword, SALT_ROUNDS);
    await prisma.user.update({
      where: { id: req.user!.id },
      data: { passwordHash },
    });

    res.json({ message: "Password changed successfully" });
  },
);

export default router;
