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

const hexColor = z.string().regex(/^#[0-9a-fA-F]{6}$/);

const bgType = z.enum(["solid", "stars", "sky", "sunset", "field"]);

export const customizationSchema = z.object({
  body: hexColor,
  stripes: hexColor,
  belly: hexColor,
  outline: hexColor,
  eyes: hexColor,
  nose: hexColor,
  bgType: bgType,
  bgColor: hexColor,
});

export type CreateDeviceInput = z.infer<typeof createDeviceSchema>;
export type UpdateDeviceInput = z.infer<typeof updateDeviceSchema>;
export type CustomizationInput = z.infer<typeof customizationSchema>;
