import { z } from "zod/v4";

export const firmwareVersionSchema = z.object({
  version: z
    .string()
    .min(1)
    .max(30)
    .regex(/^[\w.-]+$/, "Version must be alphanumeric with dots/dashes"),
});

export type FirmwareVersionInput = z.infer<typeof firmwareVersionSchema>;
