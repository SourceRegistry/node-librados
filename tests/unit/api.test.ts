import assert from "node:assert/strict";
import {test} from "node:test";
import {capabilities, CephConfirmationError, CephError, RbdFeature, RbdPool, versions} from "../../src-ts";

test("reports native ABI versions and capabilities", () => {
  assert.equal(versions.napi, 9);
  assert.equal(typeof versions.rados.major, "number");
  assert.equal(typeof versions.rbd.string, "string");
  assert.equal(typeof capabilities.radosWatch, "boolean");
});

test("exports bigint feature masks", () => {
  assert.equal(RbdFeature.Layering, 1n);
  assert.equal(RbdFeature.Journaling, 64n);
});

test("destructive operations require matching confirmation", () => {
  const pool = new RbdPool({remove: async () => undefined} as never);
  assert.throws(() => pool.remove("danger", {confirm: "other"}), CephConfirmationError);
});

test("recognizes structured errors emitted by the native addon", () => {
  const error = Object.assign(new Error("rados_read: not found"), {
    name: "CephError", code: -2, errno: 2, operation: "rados_read", resource: "object",
  });
  assert.ok(error instanceof CephError);
  assert.equal(CephError.normalize(error).resource, "object");
});
