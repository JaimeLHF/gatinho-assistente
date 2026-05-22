import type { Request, Response } from "express";
import * as eventService from "../services/eventService.js";

export async function list(req: Request, res: Response) {
  const events = await eventService.list(req.user!.id, req.query as never);
  res.json(events);
}

export async function create(req: Request, res: Response) {
  const event = await eventService.create(req.user!.id, req.body);
  res.status(201).json(event);
}

export async function getById(req: Request, res: Response) {
  const event = await eventService.getById(req.user!.id, req.params.id as string);
  res.json(event);
}

export async function update(req: Request, res: Response) {
  const event = await eventService.update(req.user!.id, req.params.id as string, req.body);
  res.json(event);
}

export async function remove(req: Request, res: Response) {
  await eventService.remove(req.user!.id, req.params.id as string);
  res.status(204).send();
}
