import api from "./client";
import type { FirmwareInfo } from "../types";

export async function listFirmwares(): Promise<FirmwareInfo[]> {
  const res = await api.get<FirmwareInfo[]>("/firmware");
  return res.data;
}

export async function uploadFirmware(version: string, file: File): Promise<FirmwareInfo> {
  const form = new FormData();
  form.append("version", version);
  form.append("file", file);
  const res = await api.post<FirmwareInfo>("/firmware", form, {
    headers: { "Content-Type": "multipart/form-data" },
  });
  return res.data;
}

export async function deleteFirmware(id: string): Promise<void> {
  await api.delete(`/firmware/${id}`);
}
