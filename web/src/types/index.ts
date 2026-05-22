export interface User {
  id: string;
  email: string;
  name: string;
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

export interface ApiError {
  error: {
    code: string;
    message: string;
  };
}
