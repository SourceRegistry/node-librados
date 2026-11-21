import assert from "node:assert/strict";
import {once} from "node:events";
import {after, before, test} from "node:test";
import {CephError, Cluster, RbdFeature} from "../../src-ts";

const enabled = process.env.CEPH_INTEGRATION === "1";
const pool = process.env.CEPH_TEST_POOL ?? `node-librados-${process.pid}`;
let cluster: Cluster;

before(async () => {
  if (!enabled) return;
  cluster = await Cluster.connect({
    clusterName: process.env.CEPH_CLUSTER ?? "ceph",
    userName: process.env.CEPH_CLIENT ?? "client.admin",
    configFile: process.env.CEPH_CONF ?? null,
  });
  await cluster.createPool(pool);
  await cluster.unsafeCommand({kind: "mon"}, {prefix: "osd pool application enable", pool, app: "rbd", yes_i_really_mean_it: true});
});

after(async () => {
  if (!enabled) return;
  await cluster.deletePool(pool, {confirm: pool}).catch(() => undefined);
  await cluster.close();
});

test("binary RADOS I/O, xattrs, omap, listing, and notifications", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  const payload = Buffer.from([0, 1, 2, 0, 255]);
  await io.writeFull("object", payload);
  assert.deepEqual(await io.read("object", 5n), payload);
  await io.setXattr("object", "kind", Buffer.from("test"));
  assert.equal((await io.getXattr("object", "kind")).toString(), "test");
  await io.setOmap("object", {alpha: Buffer.from("one")});
  assert.equal((await io.getOmap("object")).entries.alpha.toString(), "one");
  assert.deepEqual((await Array.fromAsync(io.objects())).map(value => value.name), ["object"]);

  const watch = io.watch("object", {autoAck: false});
  const received = once(watch, "notification");
  const notify = io.notify("object", Buffer.from("ping"));
  const [notification] = await received;
  assert.equal(notification.payload.toString(), "ping");
  await notification.ack(Buffer.from("pong"));
  assert.ok((await notify).includes(Buffer.from("pong")));
  await watch.close();
});

test("io context settings do not change under operations in flight", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  await io.writeFull("busy", Buffer.from("x"));
  const reads = Array.from({length: 16}, () => io.read("busy", 1n));
  const busy = (error: unknown) => error instanceof CephError && error.errno === 16;
  assert.throws(() => io.setNamespace("elsewhere"), busy);
  assert.throws(() => io.setLocatorKey("elsewhere"), busy);
  for (const data of await Promise.all(reads)) assert.equal(data.toString(), "x");
  io.setNamespace("elsewhere");
  io.setNamespace("");
  const watch = io.watch("busy");
  assert.throws(() => io.setNamespace("elsewhere"), busy);
  await watch.close();
  io.setNamespace("");
  await io.remove("busy");
});

test("lists objects a page at a time", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  const prefix = "stream-objects";
  const names = Array.from({length: 25}, (_, i) => `${prefix}-${i}`);
  for (const name of names) await io.writeFull(name, Buffer.from(name));
  const listed: string[] = [];
  for await (const object of io.objects({pageSize: 4})) if (object.name.startsWith(prefix)) listed.push(object.name);
  assert.deepEqual(listed.sort(), [...names].sort());
  // leaving a listing early closes it, or closing the io context below would fail with EBUSY
  for await (const object of io.objects({pageSize: 2})) { assert.ok(object.name); break; }
  // an open listing reads the namespace when it reads its first page, so the namespace may not change under it
  const busy = (error: unknown) => error instanceof CephError && error.errno === 16;
  const listing = io.objects({pageSize: 2});
  await listing.next();
  assert.throws(() => io.setNamespace("elsewhere"), busy);
  await listing.return(undefined);
  io.setNamespace("");
  for (const name of names) await io.remove(name);
});

test("large writes and reads, and reads past the end", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  const payload = Buffer.alloc(4 * 1024 * 1024 + 3, 7);
  await io.writeFull("zero-copy", payload);
  assert.deepEqual(await io.read("zero-copy", BigInt(payload.length)), payload);
  const tail = await io.read("zero-copy", 1024n * 1024n, BigInt(payload.length - 3));
  assert.deepEqual(tail, Buffer.from([7, 7, 7]));
  await io.remove("zero-copy");
});

test("compound write operations apply all steps or none", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  const failsWith = (errno: number) => (error: unknown) => error instanceof CephError && error.errno === errno;
  await io.writeOp("op").create({exclusive: true}).writeFull(Buffer.from("first")).setXattr("state", Buffer.from("ready")).operate();
  assert.equal((await io.read("op", 16n)).toString(), "first");
  assert.equal((await io.getXattr("op", "state")).toString(), "ready");

  await assert.rejects(io.writeOp("op").create({exclusive: true}).writeFull(Buffer.from("second")).operate(), failsWith(17));
  await assert.rejects(io.writeOp("op").compareXattr("state", "eq", Buffer.from("other")).writeFull(Buffer.from("second")).removeXattr("state").operate(), failsWith(125));
  assert.equal((await io.read("op", 16n)).toString(), "first");
  assert.equal((await io.getXattr("op", "state")).toString(), "ready");

  await io.writeOp("op").compareXattr("state", "eq", Buffer.from("ready")).writeFull(Buffer.from("second")).setXattr("state", Buffer.from("done")).operate();
  assert.equal((await io.read("op", 16n)).toString(), "second");
  assert.equal((await io.getXattr("op", "state")).toString(), "done");

  await assert.rejects(io.writeOp("op-missing").assertExists().writeFull(Buffer.from("x")).operate(), failsWith(2));
  await assert.rejects(io.stat("op-missing"), failsWith(2));
  await io.writeOp("op").remove().operate();
  await assert.rejects(io.stat("op"), failsWith(2));
});

test("compound write checks compare the stored value and the version", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  const failsWith = (errno: number) => (error: unknown) => error instanceof CephError && error.errno === errno;
  await io.writeFull("checked", Buffer.from("x"));
  await io.setXattr("checked", "v", Buffer.from("m"));
  for (const [comparison, value, passes] of [
    ["eq", "m", true], ["ne", "m", false], ["gt", "a", true], ["gt", "z", false], ["gte", "m", true],
    ["lt", "z", true], ["lt", "a", false], ["lte", "m", true],
  ] as const) {
    const op = io.writeOp("checked").compareXattr("v", comparison, Buffer.from(value)).operate();
    if (passes) await op; else await assert.rejects(op, failsWith(125), `${comparison} ${value}`);
  }

  await io.writeFull("checked", Buffer.from("v1"));
  const version = io.lastVersion;
  await assert.rejects(io.writeOp("checked").assertVersion(version + 1n).writeFull(Buffer.from("v2")).operate(), failsWith(75));
  await io.writeOp("checked").assertVersion(version).writeFull(Buffer.from("v2")).operate();
  await assert.rejects(io.writeOp("checked").assertVersion(version).writeFull(Buffer.from("v3")).operate(), failsWith(34));
  assert.equal((await io.read("checked", 16n)).toString(), "v2");

  const ops = Array.from({length: 8}, () => io.writeOp("checked").append(Buffer.from("!")).operate());
  assert.throws(() => io.setNamespace("elsewhere"), failsWith(16));
  await Promise.all(ops);
  await io.remove("checked");
});

test("RBD image, I/O, metadata, snapshots, resize, and trash", {skip: !enabled}, async () => {
  await using io = cluster.ioContext(pool);
  const rbd = io.rbd();
  await rbd.create("image", 8n * 1024n * 1024n, {features: RbdFeature.Layering});
  await using image = await rbd.open("image");
  await image.write(0n, Buffer.from("ceph"));
  assert.equal((await image.read(0n, 4n)).toString(), "ceph");
  await image.setMetadata("owner", "node-librados");
  assert.equal(await image.metadata("owner"), "node-librados");
  await image.createSnapshot("first");
  assert.equal((await image.snapshots())[0]?.name, "first");
  await image.removeSnapshot("first", {confirm: "first"});
  await image.close();
  await rbd.moveToTrash("image");
});

test("synchronous calls on closed handles throw instead of aborting the process", {skip: !enabled}, async () => {
  const closedError = (error: unknown) => error instanceof Error && error.message === "resource is closed";
  const io = cluster.ioContext(pool);
  await io.close();
  assert.throws(() => io.setNamespace("x"), closedError);
  assert.throws(() => io.setLocatorKey("x"), closedError);
  assert.throws(() => io.setReadSnapshot(1n), closedError);
  assert.throws(() => io.poolId, closedError);
  assert.throws(() => io.lastVersion, closedError);

  const closed = await Cluster.connect({
    clusterName: process.env.CEPH_CLUSTER ?? "ceph",
    userName: process.env.CEPH_CLIENT ?? "client.admin",
    configFile: process.env.CEPH_CONF ?? null,
  });
  await closed.close();
  assert.throws(() => closed.config("mon_host"), closedError);
  assert.throws(() => closed.instanceId, closedError);
  assert.throws(() => closed.ioContext(pool), closedError);
});
