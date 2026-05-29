import { Router } from "express";
import { authenticate } from "../middlewares/authenticate.js";
import { validate } from "../middlewares/validate.js";
import {
  createDeviceSchema,
  updateDeviceSchema,
  deviceParamsSchema,
  customizationSchema,
} from "../schemas/device.js";
import * as deviceController from "../controllers/deviceController.js";

const router = Router();

router.use("/devices", authenticate);

router.get("/devices", deviceController.list);
router.post("/devices", validate({ body: createDeviceSchema }), deviceController.create);
router.patch(
  "/devices/:id",
  validate({ params: deviceParamsSchema, body: updateDeviceSchema }),
  deviceController.update,
);

router.delete("/devices/:id", validate({ params: deviceParamsSchema }), deviceController.remove);

router.get(
  "/devices/:id/customization",
  validate({ params: deviceParamsSchema }),
  deviceController.getCustomization,
);
router.put(
  "/devices/:id/customization",
  validate({ params: deviceParamsSchema, body: customizationSchema }),
  deviceController.upsertCustomization,
);
router.delete(
  "/devices/:id/customization",
  validate({ params: deviceParamsSchema }),
  deviceController.deleteCustomization,
);

export default router;
