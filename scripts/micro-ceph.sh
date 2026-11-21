#!/usr/bin/env bash
# Starts a disposable single-node Ceph cluster for the integration tests: one monitor and one OSD that keeps
# its data in memory, without authentication. It is meant for containers (the devcontainer, `npm run
# test:docker` and CI), never for a host that runs a real cluster: it stops any ceph-mon and ceph-osd it finds.
#
#   scripts/micro-ceph.sh          start a fresh cluster, replacing a previous one
#   scripts/micro-ceph.sh stop     stop it
#
# The configuration is written to $CEPH_TEST_DIR/ceph.conf (default /tmp/node-librados-ceph/ceph.conf).
# Requires ceph-mon, ceph-osd and ceph-common from the Ceph release under test.
set -euo pipefail

dir=${CEPH_TEST_DIR:-/tmp/node-librados-ceph}
conf="$dir/ceph.conf"

stop() {
  pkill -x ceph-osd 2>/dev/null || true
  pkill -x ceph-mon 2>/dev/null || true
  for _ in $(seq 1 20); do
    pgrep -x ceph-osd >/dev/null || pgrep -x ceph-mon >/dev/null || return 0
    sleep 0.5
  done
}

if [ "${1:-}" = stop ]; then stop; exit 0; fi

stop
rm -rf "$dir"
mkdir -p "$dir"/{mon,osd,run,log}
ip=$(hostname -I | awk '{print $1}')

cat > "$conf" <<EOF
[global]
fsid = $(uuidgen)
mon host = $ip
run dir = $dir/run
log file = $dir/log/\$name.log
auth cluster required = none
auth service required = none
auth client required = none
osd pool default size = 1
osd pool default min size = 1
osd crush chooseleaf type = 0
mon allow pool delete = true
mon warn on pool no redundancy = false

[mon.a]
mon data = $dir/mon
mon addr = $ip
mon data avail crit = 0

[osd.0]
osd data = $dir/osd
osd objectstore = memstore
memstore device bytes = 4294967296
osd class load list = *
osd class default list = *
EOF
export CEPH_CONF="$conf"

ceph-mon --id a --mkfs --keyring /dev/null 2>/dev/null
ceph-mon --id a

osd=$(ceph osd create)
ceph osd crush add "osd.$osd" 1 root=default host=micro-ceph >/dev/null 2>&1
ceph-osd --id "$osd" --mkkey --mkfs >/dev/null 2>&1
ceph-osd --id "$osd" 2>/dev/null

for _ in $(seq 1 60); do
  if ceph osd stat 2>/dev/null | grep -q "1 up"; then
    echo "micro-ceph: cluster ready, CEPH_CONF=$conf"
    exit 0
  fi
  sleep 1
done
echo "micro-ceph: the OSD did not come up; logs are in $dir/log" >&2
exit 1
