import assert from "node:assert/strict";
import {test} from "node:test";
import {addonLoadError} from "../../src-ts/load-error";

// error messages as Node reports them, captured from real failures
const addonPath = "/app/node_modules/node-librados/bin/x86_64-linux-gnu/node-librados.node";
const fail = (code: string, message: string) => Object.assign(new Error(message), {code});

test("a missing addon file is reported as missing, not as missing Ceph libraries", () => {
  const cause = fail("MODULE_NOT_FOUND", `Cannot find module '${addonPath}'`);
  const installed = addonLoadError(cause, addonPath, true) as Error;
  assert.match(installed.message, /native addon is missing from the installed package/);
  assert.doesNotMatch(installed.message, /Install librados2/);
  assert.equal(installed.cause, cause);
  assert.match((addonLoadError(cause, addonPath, false) as Error).message, /npm run build:native/);
});

test("a Ceph client library the loader cannot find is named", () => {
  for (const library of ["librados.so.2", "librbd.so.1"]) {
    const cause = fail("ERR_DLOPEN_FAILED", `${library}: cannot open shared object file: No such file or directory`);
    const error = addonLoadError(cause, addonPath, true) as Error;
    assert.equal(error.message, `Unable to load the Ceph client library ${library}. Install librados2 and librbd1 from Ceph Squid or Tentacle.`);
    assert.equal(error.cause, cause);
  }
});

test("other load failures are passed through unchanged", () => {
  // the package path contains "librados", which must not be mistaken for a missing library
  const wrongElf = fail("ERR_DLOPEN_FAILED", `${addonPath}: wrong ELF class: ELFCLASS32`);
  assert.equal(addonLoadError(wrongElf, addonPath, true), wrongElf);
  const otherModule = fail("MODULE_NOT_FOUND", "Cannot find module 'something-else'");
  assert.equal(addonLoadError(otherModule, addonPath, true), otherModule);
  assert.equal(addonLoadError("not an error", addonPath, true), "not an error");
});
