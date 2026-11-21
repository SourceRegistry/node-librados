import path from "node:path";
import {addonLoadError} from "./load-error";

const triplets: Partial<Record<NodeJS.Architecture, string>> = {
  x64: "x86_64-linux-gnu",
  arm64: "aarch64-linux-gnu",
};

if (process.platform !== "linux") {
  throw new Error(`node-librados supports Linux only; received ${process.platform}-${process.arch}`);
}
const inPackage = __dirname.includes(`${path.sep}node_modules${path.sep}`);
const triplet = triplets[process.arch];
if (!triplet) throw new Error(`Unsupported architecture: ${process.arch}`);

const addonPath = inPackage
  ? path.join(__dirname, "..", "bin", triplet, "node-librados.node")
  : path.join(__dirname, "..", "build", process.env.NODE_ENV === "development" ? "Debug" : "Release", "node-librados.node");

try {
  // eslint-disable-next-line @typescript-eslint/no-require-imports
  module.exports = require(addonPath);
} catch (cause) {
  throw addonLoadError(cause, addonPath, inPackage);
}
