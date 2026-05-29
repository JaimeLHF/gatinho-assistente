import api from "./client";
import type { Device, DeviceWithToken, CatCustomization } from "../types";

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

export async function getCustomization(deviceId: string): Promise<CatCustomization | null> {
  const res = await api.get<CatCustomization | null>(`/devices/${deviceId}/customization`);
  return res.data;
}

export async function saveCustomization(
  deviceId: string,
  data: Omit<CatCustomization, "id" | "deviceId">,
): Promise<CatCustomization> {
  const res = await api.put<CatCustomization>(`/devices/${deviceId}/customization`, data);
  return res.data;
}

export async function deleteCustomization(deviceId: string): Promise<void> {
  await api.delete(`/devices/${deviceId}/customization`);
}
