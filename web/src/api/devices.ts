import api from "./client";
import type { Device, DeviceWithToken } from "../types";

export async function listDevices(): Promise<Device[]> {
  const res = await api.get<Device[]>("/devices");
  return res.data;
}

export async function createDevice(name: string): Promise<DeviceWithToken> {
  const res = await api.post<DeviceWithToken>("/devices", { name });
  return res.data;
}

export async function updateDevice(id: string, name: string): Promise<Device> {
  const res = await api.patch<Device>(`/devices/${id}`, { name });
  return res.data;
}

export async function deleteDevice(id: string): Promise<void> {
  await api.delete(`/devices/${id}`);
}
