/**
 * Explains why the native addon failed to load and what to do about it. Errors it does not recognize are
 * returned unchanged.
 */
export function addonLoadError(cause: unknown, addonPath: string, inPackage: boolean): unknown {
  const error = cause as NodeJS.ErrnoException | undefined;
  const message = typeof error?.message === "string" ? error.message : "";

  if (error?.code === "MODULE_NOT_FOUND" && message.includes(addonPath)) {
    return new Error(inPackage
      ? `The node-librados native addon is missing from the installed package (${addonPath}). Reinstall node-librados; if that does not help, please report a bug.`
      : `The node-librados native addon is not built (${addonPath}). Run "npm run build:native".`, {cause});
  }

  // the dynamic loader names the shared library it could not load, e.g. "librados.so.2: cannot open shared object file"
  const library = error?.code === "ERR_DLOPEN_FAILED" ? /\b(?:librados|librbd)\.so(?:\.\d+)*/.exec(message)?.[0] : undefined;
  if (library) {
    return new Error(`Unable to load the Ceph client library ${library}. Install librados2 and librbd1 from Ceph Squid or Tentacle.`, {cause});
  }

  return cause;
}
