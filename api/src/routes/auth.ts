import { Router } from "express";
import { validate } from "../middlewares/validate.js";
import { registerSchema, loginSchema, refreshSchema } from "../schemas/auth.js";
import * as authController from "../controllers/authController.js";

const router = Router();

router.post("/auth/register", validate({ body: registerSchema }), authController.register);
router.post("/auth/login", validate({ body: loginSchema }), authController.login);
router.post("/auth/refresh", validate({ body: refreshSchema }), authController.refresh);
router.post("/auth/logout", validate({ body: refreshSchema }), authController.logout);

export default router;
