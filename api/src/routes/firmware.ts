import { Router } from "express";
import multer from "multer";
import { authenticate } from "../middlewares/authenticate.js";
import { authorizeAdmin } from "../middlewares/authorizeAdmin.js";
import { authenticateDevice } from "../middlewares/authenticateDevice.js";
import { validate } from "../middlewares/validate.js";
import { firmwareVersionSchema } from "../schemas/firmware.js";
import * as firmwareController from "../controllers/firmwareController.js";

const upload = multer({
  storage: multer.memoryStorage(),
  limits: { fileSize: 4 * 1024 * 1024 }, // 4MB max
  fileFilter: (_req, file, cb) => {
    if (file.originalname.endsWith(".bin")) cb(null, true);
    else cb(new Error("Only .bin files are allowed"));
  },
});

const router = Router();

// Admin-only routes (upload, list, delete)
router.post(
  "/firmware",
  authenticate,
  authorizeAdmin,
  upload.single("file"),
  validate({ body: firmwareVersionSchema }),
  firmwareController.upload,
);
router.get("/firmware", authenticate, authorizeAdmin, firmwareController.list);
router.delete("/firmware/:id", authenticate, authorizeAdmin, firmwareController.remove);

// Device routes (check update, download binary)
router.get("/device/firmware/check", authenticateDevice, firmwareController.checkUpdate);
router.get("/device/firmware/:version/download", authenticateDevice, firmwareController.download);

export default router;
