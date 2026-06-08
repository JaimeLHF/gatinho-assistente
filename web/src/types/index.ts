export interface User {
  id: string;
  email: string;
  name: string;
  role: "ADMIN" | "USER";
  createdAt?: string;
}

export interface AuthResponse {
  user: User;
  accessToken: string;
  refreshToken: string;
}

export interface Event {
  id: string;
  userId: string;
  title: string;
  description: string | null;
  startsAt: string;
  durationMin: number | null;
  alertMinutesBefore: number;
  status: "PENDING" | "DONE" | "DISMISSED";
  createdAt: string;
  updatedAt: string;
}

export interface Device {
  id: string;
  name: string;
  lastSeenAt: string | null;
  createdAt: string;
}

export interface DeviceWithToken extends Device {
  token: string;
}

export type BgType = "solid" | "stars" | "sky" | "sunset" | "field";

export interface CatCustomization {
  id?: string;
  deviceId?: string;
  body: string;
  stripes: string;
  belly: string;
  outline: string;
  eyes: string;
  nose: string;
  bgType: BgType;
  bgColor: string;
}

export interface FirmwareInfo {
  id: string;
  version: string;
  size: number;
  createdAt: string;
}

export interface ApiError {
  error: {
    code: string;
    message: string;
  };
}
