import { z } from "zod/v4";

export const createEventSchema = z.object({
  title: z.string().min(1).max(200),
  description: z.string().max(2000).optional(),
  startsAt: z.coerce.date(),
  durationMin: z.number().int().positive().max(10080).optional(), // max 1 week
  alertMinutesBefore: z.number().int().min(0).max(1440).default(5), // max 24h
});

export const updateEventSchema = z.object({
  title: z.string().min(1).max(200).optional(),
  description: z.string().max(2000).nullable().optional(),
  startsAt: z.coerce.date().optional(),
  durationMin: z.number().int().positive().max(10080).nullable().optional(),
  alertMinutesBefore: z.number().int().min(0).max(1440).optional(),
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
