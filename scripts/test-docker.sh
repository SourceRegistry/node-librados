#!/usr/bin/env bash
# Builds the addon and runs the type check, unit tests, integration tests and API coverage check in the
# devcontainer image, against a disposable cluster started inside the container. Works on any host with
# Docker, including macOS. Nothing is written to the working tree: node_modules and build/ live in volumes.
#
#   npm run test:docker                        Ceph Squid
#   CEPH_RELEASE=tentacle npm run test:docker  Ceph Tentacle
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
release=${CEPH_RELEASE:-squid}
image="node-librados-dev:$release"

docker build --quiet --tag "$image" --build-arg "CEPH_RELEASE=$release" "$root/.devcontainer" >/dev/null

tty=()
[ -t 1 ] && tty=(--tty)
docker run --rm ${tty[@]+"${tty[@]}"} \
  --volume "$root:/workspaces/node-librados" \
  --volume "node-librados-node-modules:/workspaces/node-librados/node_modules" \
  --volume "node-librados-build-$release:/workspaces/node-librados/build" \
  --workdir /workspaces/node-librados \
  --env CEPH_INTEGRATION=1 \
  --env CEPH_CONF=/tmp/node-librados-ceph/ceph.conf \
  "$image" bash -euo pipefail -c '
    scripts/micro-ceph.sh
    npm ci --no-audit --no-fund
    npm run build:native
    npm run check
    npm test
    npm run coverage:api
  '
