import type { Request, Response } from "express";
import * as deviceService from "../services/deviceService.js";

export async function list(req: Request, res: Response) {
  const devices = await deviceService.list(req.user!.id);
  res.json(devices);
}

export async function create(req: Request, res: Response) {
  const result = await deviceService.create(req.user!.id, req.body);
  res.status(201).json(result);
}

export async function update(req: Request, res: Response) {
  const device = await deviceService.update(req.user!.id, req.params.id as string, req.body);
  res.json(device);
}

export async function remove(req: Request, res: Response) {
  await deviceService.remove(req.user!.id, req.params.id as string);
  res.status(204).send();
}

export async function getCustomization(req: Request, res: Response) {
  const customization = await deviceService.getCustomization(req.user!.id, req.params.id as string);
  res.json(customization);
}

export async function upsertCustomization(req: Request, res: Response) {
  const customization = await deviceService.upsertCustomization(
    req.user!.id,
    req.params.id as string,
    req.body,
  );
  res.json(customization);
}

export async function deleteCustomization(req: Request, res: Response) {
  await deviceService.deleteCustomization(req.user!.id, req.params.id as string);
  res.status(204).send();
}

export async function nextEvent(req: Request, res: Response) {
  const { event, customization } = await deviceService.nextEvent(
    req.device!.id,
    req.device!.userId,
  );
  res.json({
    event: event ?? null,
    customization: customization ?? null,
    currentTime: new Date().toISOString(),
  });
}
