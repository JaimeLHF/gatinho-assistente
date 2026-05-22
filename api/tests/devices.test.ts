import { describe, it, expect, beforeAll, afterAll } from "vitest";
import supertest from "supertest";
import app from "../src/app.js";
import { prisma } from "../src/lib/prisma.js";

const request = supertest(app);

const TEST_USER = {
  email: "devices-test@test.com",
  password: "password123",
  name: "Device Tester",
};

let accessToken: string;
let deviceId: string;
let deviceToken: string;

beforeAll(async () => {
  const res = await request.post("/api/v1/auth/register").send(TEST_USER);
  accessToken = res.body.accessToken;
});

afterAll(async () => {
  await prisma.event.deleteMany({
    where: { user: { email: TEST_USER.email } },
  });
  await prisma.device.deleteMany({
    where: { user: { email: TEST_USER.email } },
  });
  await prisma.refreshToken.deleteMany({
    where: { user: { email: TEST_USER.email } },
  });
  await prisma.user.deleteMany({
    where: { email: TEST_USER.email },
  });
  await prisma.$disconnect();
});

describe("Devices CRUD", () => {
  it("POST /api/v1/devices — creates device and returns token once", async () => {
    const res = await request
      .post("/api/v1/devices")
      .set("Authorization", `Bearer ${accessToken}`)
      .send({ name: "Living Room ESP32" })
      .expect(201);

    expect(res.body.name).toBe("Living Room ESP32");
    expect(res.body.id).toEqual(expect.any(String));
    expect(res.body.token).toEqual(expect.any(String));
    expect(res.body.token.length).toBe(64); // 32 bytes hex

    deviceId = res.body.id;
    deviceToken = res.body.token;
  });

  it("GET /api/v1/devices — lists devices without token field", async () => {
    const res = await request
      .get("/api/v1/devices")
      .set("Authorization", `Bearer ${accessToken}`)
      .expect(200);

    expect(res.body).toHaveLength(1);
    expect(res.body[0].name).toBe("Living Room ESP32");
    expect(res.body[0]).not.toHaveProperty("token");
    expect(res.body[0]).not.toHaveProperty("tokenHash");
  });

  it("POST /api/v1/devices — 401 without JWT", async () => {
    await request
      .post("/api/v1/devices")
      .send({ name: "No auth" })
      .expect(401);
  });

  it("DELETE /api/v1/devices/:id — deletes device", async () => {
    // Create a second device to delete
    const createRes = await request
      .post("/api/v1/devices")
      .set("Authorization", `Bearer ${accessToken}`)
      .send({ name: "Temp Device" })
      .expect(201);

    await request
      .delete(`/api/v1/devices/${createRes.body.id}`)
      .set("Authorization", `Bearer ${accessToken}`)
      .expect(204);

    const listRes = await request
      .get("/api/v1/devices")
      .set("Authorization", `Bearer ${accessToken}`)
      .expect(200);

    expect(listRes.body).toHaveLength(1);
  });
});

describe("Device polling", () => {
  it("GET /api/v1/device/next-event — 401 without X-Device-Token", async () => {
    const res = await request.get("/api/v1/device/next-event").expect(401);
    expect(res.body.error.code).toBe("MISSING_DEVICE_TOKEN");
  });

  it("GET /api/v1/device/next-event — 401 with invalid token", async () => {
    const res = await request
      .get("/api/v1/device/next-event")
      .set("X-Device-Token", "invalid-token")
      .expect(401);

    expect(res.body.error.code).toBe("INVALID_DEVICE_TOKEN");
  });

  it("GET /api/v1/device/next-event — returns null when no pending events", async () => {
    const res = await request
      .get("/api/v1/device/next-event")
      .set("X-Device-Token", deviceToken)
      .expect(200);

    expect(res.body.event).toBeNull();
    expect(res.body.currentTime).toEqual(expect.any(String));
  });

  it("GET /api/v1/device/next-event — returns next pending event", async () => {
    // Create a future event
    const futureDate = new Date(Date.now() + 60 * 60 * 1000).toISOString();
    await request
      .post("/api/v1/events")
      .set("Authorization", `Bearer ${accessToken}`)
      .send({
        title: "Feed the cat",
        startsAt: futureDate,
        alertMinutesBefore: 10,
      })
      .expect(201);

    const res = await request
      .get("/api/v1/device/next-event")
      .set("X-Device-Token", deviceToken)
      .expect(200);

    expect(res.body.event).not.toBeNull();
    expect(res.body.event.title).toBe("Feed the cat");
    expect(res.body.event.alertMinutesBefore).toBe(10);
    expect(res.body.currentTime).toEqual(expect.any(String));
  });

  it("GET /api/v1/device/next-event — updates lastSeenAt", async () => {
    const devices = await request
      .get("/api/v1/devices")
      .set("Authorization", `Bearer ${accessToken}`)
      .expect(200);

    expect(devices.body[0].lastSeenAt).not.toBeNull();
  });
});
