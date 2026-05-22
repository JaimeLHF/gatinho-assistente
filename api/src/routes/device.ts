import { Router } from "express";
import { authenticateDevice } from "../middlewares/authenticateDevice.js";
import * as deviceController from "../controllers/deviceController.js";

const router = Router();

router.get("/device/next-event", authenticateDevice, deviceController.nextEvent);

export default router;
