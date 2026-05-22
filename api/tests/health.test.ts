import { describe, it, expect } from "vitest";
import supertest from "supertest";
import app from "../src/app.js";

const request = supertest(app);

describe("Health", () => {
  it("GET /api/v1/health — returns ok", async () => {
    const res = await request.get("/api/v1/health").expect(200);

    expect(res.body.status).toBe("ok");
    expect(res.body.time).toEqual(expect.any(String));
  });
});
