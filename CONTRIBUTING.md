# Contributing to node-librados

Thanks for helping! Bug reports, fixes, new bindings and documentation are all welcome. This guide covers how
to report a problem, set up a development environment, and get a pull request merged.

## Reporting a problem

- **Bugs:** open a [bug report](https://github.com/SourceRegistry/node-librados/issues/new?template=bug_report.yml).
  The form asks for a diagnostics snippet, the Ceph versions and a minimal reproduction; with those we can
  usually reproduce the problem against a test cluster straight away.
- **Missing APIs or changes:** open a [feature request](https://github.com/SourceRegistry/node-librados/issues/new?template=feature_request.yml).
  For anything larger than a small binding, please open the issue before writing code so we can agree on the API.
- **Security issues:** do not open a public issue. Use *Security → Report a vulnerability* on this repository.

## Development environment

The addon builds against librados and librbd on Linux only. Pick one of three setups; all of them run the same
tests against a disposable single-node Ceph cluster, so you never need a real cluster.

### 1. Devcontainer (recommended)

Open the repository in VS Code, JetBrains Gateway or GitHub Codespaces and reopen it in the container. It installs
Node 24, the Ceph Squid client libraries and a monitor and OSD, builds the addon, and starts the test cluster
every time the container starts. Then:

```sh
npm run build   # native addon + TypeScript
npm test        # unit and integration tests
```

`CEPH_INTEGRATION` and `CEPH_CONF` are already set in the container.

### 2. Docker, from any host (including macOS)

```sh
npm run test:docker                        # Ceph Squid
CEPH_RELEASE=tentacle npm run test:docker  # Ceph Tentacle
```

This builds the devcontainer image, starts the test cluster inside it, and runs the native build, type check,
unit and integration tests and the API coverage check. `node_modules` and `build/` live in Docker volumes, so
your working tree is not touched.

### 3. Your own Linux machine

Install Node 22 or 24, a C++17 compiler, `python3`, `make` and `librados-dev`/`librbd-dev` from
[download.ceph.com](https://docs.ceph.com/en/latest/install/get-packages/) (Squid or Tentacle). Then:

```sh
npm ci
npm run build
npm run test:unit
```

For the integration tests, either run them in a container as above, or point them at a disposable cluster you
control:

```sh
CEPH_INTEGRATION=1 CEPH_CONF=/path/to/ceph.conf CEPH_CLIENT=client.admin npm run test:integration
```

The suite creates and deletes a uniquely named pool, so the identity needs pool-management permissions. Never
point it at a production cluster. `scripts/micro-ceph.sh` starts the same throwaway cluster the containers use,
but it stops every `ceph-mon` and `ceph-osd` on the machine, so only run it inside a container.

## Project layout

| Path | What it holds |
|---|---|
| `src/` | The N-API addon in C++17. `common.h` has the shared handle state, the async worker and argument helpers |
| `src-ts/` | The TypeScript API that users import, and the typings of the native addon (`native.ts`) |
| `tests/unit/` | Tests that run without a cluster |
| `tests/integration/` | Tests against a cluster; skipped unless `CEPH_INTEGRATION=1` |
| `coverage/api.json` | Every librados and librbd symbol the addon calls; `npm run coverage:api` checks it |
| `scripts/` | Coverage and prebuild checks, the test cluster and the Docker test runner |

## Writing native code

These rules come from real bugs; reviews check for them.

- **Run blocking librados calls on the thread pool** with `async_void()` or `async_value()`. Never call into
  librados on the main thread except for calls that only read local state.
- **Count operations on an `IoContext` as in flight.** Capture `auto state = in_flight(state_);` instead of
  `state_` in every operation that uses the io context (and `in_flight(io_)` in `RbdPoolWrap`). Operations read
  the namespace, locator key and read snapshot when they run, so the setters refuse with `EBUSY` while anything
  is in flight. Long-lived objects that use the io context, like watches and object listings, call
  `begin_op()`/`end_op()` themselves.
- **Report errors properly.** On the main thread, throw a `Napi::Error` or call `throw_ceph()` for a negative
  errno. In a worker, return the negative errno and let the worker turn it into a `CephError`.
- **Mind lifetimes.** A worker and everything its lambda captures is destroyed on the main thread after the
  promise settles, so capturing an N-API reference there is safe (see `BufferView`). Anything used on the thread
  pool must be owned by the lambda, never borrowed from the `CallbackInfo`.
- **Keep the coverage manifest exact.** Every new `rados_*`/`rbd_*` call goes into `coverage/api.json`.
- **Build without warnings.** CI compiles with `-Wall -Wextra -Wpedantic` and also with the newest GCC, which
  rejects missing standard library includes.

## Tests

- Every change comes with a test. A bug fix comes with a test that fails without the fix.
- Integration tests create objects and images in the shared test pool: give them unique names and remove them at
  the end of the test.
- Use `await using` for io contexts and images so they are closed even when an assertion fails.

## Commits and pull requests

1. Fork the repository and create a branch from `main`.
2. Make your change with tests, then run `npm run check`, `npm test` and `npm run coverage:api`
   (or `npm run test:docker`, which runs all of them).
3. Open a pull request and fill in the template. Keep "Allow edits by maintainers" on, so we can rebase it for you
   when `main` moves.

Pull requests are squash merged, and [semantic-release](https://semantic-release.gitbook.io/) publishes a new npm
version from the merged commit message. That message is the **PR title**, so it must follow
[Conventional Commits](https://www.conventionalcommits.org/); a CI check enforces this:

| Title | Release |
|---|---|
| `fix(rados): ...`, `perf(rbd): ...` | patch |
| `feat(rados): ...` | minor |
| `feat(rados)!: ...` or a `BREAKING CHANGE:` footer | major |
| `docs: ...`, `test: ...`, `ci: ...`, `chore: ...`, `refactor: ...`, `build: ...` | none |

Common scopes are `rados`, `rbd`, `native`, `build`, `ci` and `deps`.

## What CI runs

| Job | What it checks |
|---|---|
| `build` | Native build, type check and unit tests on Node 22 and 24, Ceph Squid and Tentacle, x64 and arm64 |
| `integration` | The integration suite against the micro cluster, on Squid and Tentacle |
| `newest-compiler` | Every source file compiles with GCC 15 |
| `PR title` | The title follows Conventional Commits |
| `Ceph integration` | Nightly, the integration suite against a MicroCeph cluster |

Workflows on a first pull request from a new contributor wait for a maintainer to approve them.

## License

By contributing you agree that your contributions are licensed under the [Apache License 2.0](LICENSE), the
license of this project.
