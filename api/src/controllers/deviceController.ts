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

export async function nextEvent(req: Request, res: Response) {
  const event = await deviceService.nextEvent(req.device!.userId);
  res.json({
    event: event ?? null,
    currentTime: new Date().toISOString(),
  });
}
