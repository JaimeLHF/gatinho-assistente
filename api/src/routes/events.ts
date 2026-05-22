import { Router } from "express";
import { authenticate } from "../middlewares/authenticate.js";
import { validate } from "../middlewares/validate.js";
import {
  createEventSchema,
  updateEventSchema,
  listEventsQuery,
  eventParamsSchema,
} from "../schemas/event.js";
import * as eventController from "../controllers/eventController.js";

const router = Router();

router.use("/events", authenticate);

router.get("/events", validate({ query: listEventsQuery }), eventController.list);
router.post("/events", validate({ body: createEventSchema }), eventController.create);

router.get(
  "/events/:id",
  validate({ params: eventParamsSchema }),
  eventController.getById,
);

router.patch(
  "/events/:id",
  validate({ params: eventParamsSchema, body: updateEventSchema }),
  eventController.update,
);

router.delete(
  "/events/:id",
  validate({ params: eventParamsSchema }),
  eventController.remove,
);

export default router;
