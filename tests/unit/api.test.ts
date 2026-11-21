import assert from "node:assert/strict";
import {test} from "node:test";
import {capabilities, CephConfirmationError, CephError, Cluster, RbdFeature, RbdPool, versions} from "../../src-ts";

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

test("normalize() turns error-like objects into a CephError and keeps real ones", () => {
  const plain = Object.assign(new Error("busy"), {code: -16, operation: "rados_ioctx_set_namespace"});
  const normalized = CephError.normalize(plain);
  assert.equal(Object.getPrototypeOf(normalized), CephError.prototype);
  assert.equal(normalized.errno, 16);
  assert.equal(normalized.code, -16);
  assert.equal(normalized.message, "busy");
  assert.equal(normalized.operation, "rados_ioctx_set_namespace");
  assert.equal(normalized.cause, plain);

  const real = new CephError("busy", {code: -16, errno: 16, operation: "op"});
  assert.equal(CephError.normalize(real), real);
  assert.throws(() => CephError.normalize(new Error("other")), /other/);
});

test("native errors name their errno", async () => {
  await assert.rejects(Cluster.connect({configFile: "/nonexistent/node-librados/ceph.conf", parseEnvironment: false}), (error: unknown) => {
    assert.ok(error instanceof CephError);
    assert.equal(error.code, -2);
    assert.equal((error as CephError).codeName, "ENOENT");
    assert.equal(CephError.normalize(error).codeName, "ENOENT");
    return true;
  });
});
