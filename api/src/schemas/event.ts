import { z } from "zod/v4";

export const createEventSchema = z.object({
  title: z.string().min(1),
  description: z.string().optional(),
  startsAt: z.coerce.date(),
  durationMin: z.number().int().positive().optional(),
  alertMinutesBefore: z.number().int().min(0).default(5),
});

export const updateEventSchema = z.object({
  title: z.string().min(1).optional(),
  description: z.string().nullable().optional(),
  startsAt: z.coerce.date().optional(),
  durationMin: z.number().int().positive().nullable().optional(),
  alertMinutesBefore: z.number().int().min(0).optional(),
  status: z.enum(["PENDING", "DONE", "DISMISSED"]).optional(),
});

export const listEventsQuery = z.object({
  from: z.coerce.date().optional(),
  to: z.coerce.date().optional(),
});

export const eventParamsSchema = z.object({
  id: z.string().min(1),
});

export type CreateEventInput = z.infer<typeof createEventSchema>;
export type UpdateEventInput = z.infer<typeof updateEventSchema>;
export type ListEventsQuery = z.infer<typeof listEventsQuery>;
