import fs from "node:fs";

const manifest = JSON.parse(fs.readFileSync(new URL("../coverage/api.json", import.meta.url), "utf8"));
const sources = fs.readdirSync(new URL("../src", import.meta.url)).filter(name => name.endsWith(".cpp"))
  .map(name => fs.readFileSync(new URL(`../src/${name}`, import.meta.url), "utf8")).join("\n");

const classified = new Set([...manifest.rados, ...manifest.rbd]);
const used = new Set([...sources.matchAll(/\b(?:rados|rbd)_[a-zA-Z0-9_]+(?=\s*\()/g)].map(match => match[0]));
const stale = [...classified].filter(symbol => !used.has(symbol));
const unclassified = [...used].filter(symbol => !classified.has(symbol));
if (stale.length || unclassified.length) {
  if (stale.length) console.error(`Coverage manifest contains symbols absent from the addon:\n${stale.join("\n")}`);
  if (unclassified.length) console.error(`Addon uses unclassified symbols:\n${unclassified.join("\n")}`);
  process.exit(1);
}
console.log(`Verified exact coverage for ${manifest.rados.length} librados and ${manifest.rbd.length} librbd symbols.`);
