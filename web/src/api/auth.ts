import api from "./client";
import type { AuthResponse, User } from "../types";

interface RegisterData {
  email: string;
  password: string;
  name: string;
}

interface LoginData {
  email: string;
  password: string;
}

export async function register(data: RegisterData): Promise<AuthResponse> {
  const res = await api.post<AuthResponse>("/auth/register", data);
  return res.data;
}

export async function login(data: LoginData): Promise<AuthResponse> {
  const res = await api.post<AuthResponse>("/auth/login", data);
  return res.data;
}

export async function refresh(refreshToken: string): Promise<{ accessToken: string; refreshToken: string }> {
  const res = await api.post<{ accessToken: string; refreshToken: string }>("/auth/refresh", { refreshToken });
  return res.data;
}

export async function logout(refreshToken: string): Promise<void> {
  await api.post("/auth/logout", { refreshToken });
}

export async function getMe(): Promise<User> {
  const res = await api.get<User>("/me");
  return res.data;
}

export async function updateProfile(data: { name?: string; email?: string }): Promise<User> {
  const res = await api.patch<User>("/me", data);
  return res.data;
}

export async function changePassword(currentPassword: string, newPassword: string): Promise<void> {
  await api.post("/me/change-password", { currentPassword, newPassword });
}
