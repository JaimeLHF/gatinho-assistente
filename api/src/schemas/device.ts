import { z } from "zod/v4";

export const createDeviceSchema = z.object({
  name: z.string().min(1).max(100),
});

export const deviceParamsSchema = z.object({
  id: z.string().min(1),
});

export const updateDeviceSchema = z.object({
  name: z.string().min(1).max(100),
});

export type CreateDeviceInput = z.infer<typeof createDeviceSchema>;
export type UpdateDeviceInput = z.infer<typeof updateDeviceSchema>;
