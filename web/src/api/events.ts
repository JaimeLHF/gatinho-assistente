import api from "./client";
import type { Event } from "../types";

interface CreateEventData {
  title: string;
  description?: string;
  startsAt: string;
  durationMin?: number;
  alertMinutesBefore?: number;
}

interface UpdateEventData {
  title?: string;
  description?: string | null;
  startsAt?: string;
  durationMin?: number | null;
  alertMinutesBefore?: number;
  status?: "PENDING" | "DONE" | "DISMISSED";
}

interface ListParams {
  from?: string;
  to?: string;
}

export async function listEvents(params?: ListParams): Promise<Event[]> {
  const res = await api.get<Event[]>("/events", { params });
  return res.data;
}

export async function createEvent(data: CreateEventData): Promise<Event> {
  const res = await api.post<Event>("/events", data);
  return res.data;
}

export async function getEvent(id: string): Promise<Event> {
  const res = await api.get<Event>(`/events/${id}`);
  return res.data;
}

export async function updateEvent(id: string, data: UpdateEventData): Promise<Event> {
  const res = await api.patch<Event>(`/events/${id}`, data);
  return res.data;
}

export async function deleteEvent(id: string): Promise<void> {
  await api.delete(`/events/${id}`);
}
