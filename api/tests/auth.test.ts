import { describe, it, expect, afterAll } from "vitest";
import supertest from "supertest";
import app from "../src/app.js";
import { prisma } from "../src/lib/prisma.js";

const request = supertest(app);

const TEST_USER = {
  email: "auth-test@example.com",
  password: "securepass123",
  name: "Auth Test",
};

let refreshToken: string;

describe("Auth", () => {
  afterAll(async () => {
    await prisma.refreshToken.deleteMany({
      where: { user: { email: TEST_USER.email } },
    });
    await prisma.user.deleteMany({ where: { email: TEST_USER.email } });
    await prisma.$disconnect();
  });

  it("POST /api/v1/auth/register — happy path", async () => {
    const res = await request
      .post("/api/v1/auth/register")
      .send(TEST_USER)
      .expect(201);

    expect(res.body.user.email).toBe(TEST_USER.email);
    expect(res.body.user.name).toBe(TEST_USER.name);
    expect(res.body.user).not.toHaveProperty("passwordHash");
    expect(res.body.accessToken).toEqual(expect.any(String));
    expect(res.body.refreshToken).toEqual(expect.any(String));

    refreshToken = res.body.refreshToken;
  });

  it("POST /api/v1/auth/login — wrong password returns 401", async () => {
    const res = await request
      .post("/api/v1/auth/login")
      .send({ email: TEST_USER.email, password: "wrongpassword" })
      .expect(401);

    expect(res.body.error.code).toBe("INVALID_CREDENTIALS");
  });

  it("POST /api/v1/auth/refresh — valid refresh rotates tokens", async () => {
    const res = await request
      .post("/api/v1/auth/refresh")
      .send({ refreshToken })
      .expect(200);

    expect(res.body.accessToken).toEqual(expect.any(String));
    expect(res.body.refreshToken).toEqual(expect.any(String));
    expect(res.body.refreshToken).not.toBe(refreshToken);

    // old token should be revoked
    await request
      .post("/api/v1/auth/refresh")
      .send({ refreshToken })
      .expect(401);

    refreshToken = res.body.refreshToken;
  });

  it("GET /api/v1/me — without token returns 401", async () => {
    const res = await request.get("/api/v1/me").expect(401);

    expect(res.body.error.code).toBe("UNAUTHORIZED");
  });

  it("GET /api/v1/me — with valid token returns user", async () => {
    const loginRes = await request
      .post("/api/v1/auth/login")
      .send({ email: TEST_USER.email, password: TEST_USER.password })
      .expect(200);

    const res = await request
      .get("/api/v1/me")
      .set("Authorization", `Bearer ${loginRes.body.accessToken}`)
      .expect(200);

    expect(res.body.email).toBe(TEST_USER.email);
    expect(res.body.name).toBe(TEST_USER.name);
    expect(res.body).not.toHaveProperty("passwordHash");
  });
});
