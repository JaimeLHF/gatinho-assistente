import { prisma } from "../lib/prisma.js";
import { ApiError } from "../middlewares/errorHandler.js";
import type {
  CreateEventInput,
  UpdateEventInput,
  ListEventsQuery,
} from "../schemas/event.js";

export async function list(userId: string, query: ListEventsQuery) {
  const where: Record<string, unknown> = { userId };

  if (query.from || query.to) {
    const startsAt: Record<string, Date> = {};
    if (query.from) startsAt.gte = query.from;
    if (query.to) startsAt.lte = query.to;
    where.startsAt = startsAt;
  }

  return prisma.event.findMany({
    where,
    orderBy: { startsAt: "asc" },
  });
}

export async function create(userId: string, data: CreateEventInput) {
  return prisma.event.create({
    data: { ...data, userId },
  });
}

export async function getById(userId: string, eventId: string) {
  const event = await prisma.event.findUnique({ where: { id: eventId } });

  if (!event || event.userId !== userId) {
    throw new ApiError(404, "EVENT_NOT_FOUND", "Event not found");
  }

  return event;
}

export async function update(
  userId: string,
  eventId: string,
  data: UpdateEventInput,
) {
  await getById(userId, eventId);

  return prisma.event.update({
    where: { id: eventId },
    data,
  });
}

export async function remove(userId: string, eventId: string) {
  await getById(userId, eventId);

  await prisma.event.delete({ where: { id: eventId } });
}
