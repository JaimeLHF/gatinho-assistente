import type { Request, Response } from "express";
import * as firmwareService from "../services/firmwareService.js";

export async function upload(req: Request, res: Response) {
  if (!req.file) {
    res.status(400).json({ error: { code: "NO_FILE", message: "No firmware file uploaded" } });
    return;
  }

  const { version } = req.body as { version: string };
  const firmware = await firmwareService.upload(req.user!.id, version, req.file);
  res.status(201).json(firmware);
}

export async function list(req: Request, res: Response) {
  const firmwares = await firmwareService.list();
  res.json(firmwares);
}

export async function latest(_req: Request, res: Response) {
  const fw = await firmwareService.latest();
  res.json(fw);
}

export async function download(req: Request, res: Response) {
  const version = req.params.version as string;
  const filePath = await firmwareService.getFilePath(version);
  res.download(filePath, `firmware-${version}.bin`);
}

export async function remove(req: Request, res: Response) {
  await firmwareService.remove(req.user!.id, req.params.id as string);
  res.status(204).send();
}

// Device endpoint: check if update available
export async function checkUpdate(req: Request, res: Response) {
  const currentVersion = req.headers["x-firmware-version"] as string | undefined;
  const fw = await firmwareService.latest();

  if (!fw || fw.version === currentVersion) {
    res.json({ update: false });
    return;
  }

  const baseUrl = `${req.protocol}://${req.get("host")}`;
  res.json({
    update: true,
    version: fw.version,
    size: fw.size,
    url: `${baseUrl}/api/v1/device/firmware/${fw.version}/download`,
  });
}
