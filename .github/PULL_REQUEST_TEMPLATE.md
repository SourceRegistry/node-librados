<!--
Thanks for contributing! The PR title becomes the squash-merge commit message and decides the next release, so
it must follow Conventional Commits, e.g. `fix(rados): handle short reads` or `feat(rbd): bind image groups`.
See CONTRIBUTING.md.
-->

## What and why

<!-- What does this change, and what problem does it solve? Link the issue: "Fixes #123". -->

## How it was tested

<!-- Which tests did you add or run? Against which Ceph release (`npm run test:docker` runs Squid by default)? -->

## Checklist

- [ ] `npm run check`, `npm test` (with the integration suite) and `npm run coverage:api` pass
- [ ] New behaviour has a test; a bug fix has a test that fails without the fix
- [ ] New librados/librbd symbols are classified in `coverage/api.json`
- [ ] Operations on an `IoContext` use `in_flight(state_)` (see CONTRIBUTING.md)
- [ ] README updated for user-visible changes
- [ ] Breaking changes are marked with `!` in the title and described above
