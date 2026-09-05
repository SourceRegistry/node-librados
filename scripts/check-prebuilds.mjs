import fs from "node:fs";

const binaries = new Map([
  ["bin/x86_64-linux-gnu/node-librados.node", 62],
  ["bin/aarch64-linux-gnu/node-librados.node", 183],
]);

for (const [path, expectedMachine] of binaries) {
  let header;
  try {
    header = fs.readFileSync(new URL(`../${path}`, import.meta.url)).subarray(0, 20);
  } catch (cause) {
    throw new Error(`Missing required prebuilt addon: ${path}`, {cause});
  }
  if (header.length < 20 || !header.subarray(0, 4).equals(Buffer.from([0x7f, 0x45, 0x4c, 0x46]))) {
    throw new Error(`${path} is not an ELF binary`);
  }
  const littleEndian = header[5] === 1;
  const machine = littleEndian ? header.readUInt16LE(18) : header.readUInt16BE(18);
  if (machine !== expectedMachine) {
    throw new Error(`${path} has ELF machine ${machine}; expected ${expectedMachine}`);
  }
}

console.log("Verified Linux x86-64 and ARM64 prebuilt addons.");
