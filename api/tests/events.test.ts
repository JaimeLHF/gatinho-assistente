import { describe, it, expect, beforeAll, afterAll } from "vitest";
import supertest from "supertest";
import app from "../src/app.js";
import { prisma } from "../src/lib/prisma.js";

const request = supertest(app);

const USER_A = {
  email: "events-a@test.com",
  password: "password123",
  name: "User A",
};

const USER_B = {
  email: "events-b@test.com",
  password: "password123",
  name: "User B",
};

let tokenA: string;
let tokenB: string;
let eventId: string;

beforeAll(async () => {
  const resA = await request.post("/api/v1/auth/register").send(USER_A);
  tokenA = resA.body.accessToken;

  const resB = await request.post("/api/v1/auth/register").send(USER_B);
  tokenB = resB.body.accessToken;
});

afterAll(async () => {
  await prisma.event.deleteMany({
    where: { user: { email: { in: [USER_A.email, USER_B.email] } } },
  });
  await prisma.refreshToken.deleteMany({
    where: { user: { email: { in: [USER_A.email, USER_B.email] } } },
  });
  await prisma.user.deleteMany({
    where: { email: { in: [USER_A.email, USER_B.email] } },
  });
  await prisma.$disconnect();
});

describe("Events CRUD", () => {
  it("POST /api/v1/events — creates event", async () => {
    const res = await request
      .post("/api/v1/events")
      .set("Authorization", `Bearer ${tokenA}`)
      .send({
        title: "Vet appointment",
        description: "Annual checkup",
        startsAt: "2026-06-01T10:00:00Z",
        durationMin: 30,
        alertMinutesBefore: 10,
      })
      .expect(201);

    expect(res.body.title).toBe("Vet appointment");
    expect(res.body.status).toBe("PENDING");
    expect(res.body.id).toEqual(expect.any(String));
    eventId = res.body.id;
  });

  it("GET /api/v1/events — lists user events", async () => {
    const res = await request
      .get("/api/v1/events")
      .set("Authorization", `Bearer ${tokenA}`)
      .expect(200);

    expect(res.body).toHaveLength(1);
    expect(res.body[0].title).toBe("Vet appointment");
  });

  it("GET /api/v1/events — filters by from/to", async () => {
    // Outside range — should return empty
    const res = await request
      .get("/api/v1/events")
      .query({ from: "2027-01-01T00:00:00Z" })
      .set("Authorization", `Bearer ${tokenA}`)
      .expect(200);

    expect(res.body).toHaveLength(0);
  });

  it("GET /api/v1/events/:id — returns event", async () => {
    const res = await request
      .get(`/api/v1/events/${eventId}`)
      .set("Authorization", `Bearer ${tokenA}`)
      .expect(200);

    expect(res.body.id).toBe(eventId);
    expect(res.body.description).toBe("Annual checkup");
  });

  it("GET /api/v1/events/:id — 404 for other user", async () => {
    const res = await request
      .get(`/api/v1/events/${eventId}`)
      .set("Authorization", `Bearer ${tokenB}`)
      .expect(404);

    expect(res.body.error.code).toBe("EVENT_NOT_FOUND");
  });

  it("PATCH /api/v1/events/:id — updates event", async () => {
    const res = await request
      .patch(`/api/v1/events/${eventId}`)
      .set("Authorization", `Bearer ${tokenA}`)
      .send({ title: "Updated title", status: "DONE" })
      .expect(200);

    expect(res.body.title).toBe("Updated title");
    expect(res.body.status).toBe("DONE");
  });

  it("DELETE /api/v1/events/:id — deletes event", async () => {
    await request
      .delete(`/api/v1/events/${eventId}`)
      .set("Authorization", `Bearer ${tokenA}`)
      .expect(204);

    await request
      .get(`/api/v1/events/${eventId}`)
      .set("Authorization", `Bearer ${tokenA}`)
      .expect(404);
  });

  it("POST /api/v1/events — 401 without token", async () => {
    await request
      .post("/api/v1/events")
      .send({ title: "No auth", startsAt: "2026-06-01T10:00:00Z" })
      .expect(401);
  });
});
