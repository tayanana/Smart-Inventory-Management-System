#!/usr/bin/env bash
# Smoke test: add a part, save/quit, restart, search finds it.
set -euo pipefail

BIN="${1:?path to AutoInventorySystem binary}"
WORKDIR="$(mktemp -d "${TMPDIR:-/tmp}/autoinv-smoke.XXXXXX")"
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

"$BIN" "$WORKDIR" >"$WORKDIR/run1.out" <<'EOF'
1
BRAKE_PAD
Front brake pad
10

11
EOF

if [[ ! -f "$WORKDIR/inventory.csv" ]]; then
  echo "smoke_persist: inventory.csv was not written" >&2
  cat "$WORKDIR/run1.out" >&2
  exit 1
fi

if ! grep -q "BRAKE_PAD" "$WORKDIR/inventory.csv"; then
  echo "smoke_persist: BRAKE_PAD missing from inventory.csv" >&2
  cat "$WORKDIR/inventory.csv" >&2
  exit 1
fi

"$BIN" "$WORKDIR" >"$WORKDIR/run2.out" <<'EOF'
5
BRAKE_PAD
11
EOF

if ! grep -q "BRAKE_PAD" "$WORKDIR/run2.out"; then
  echo "smoke_persist: restart search did not find BRAKE_PAD" >&2
  echo "---- run1 ----" >&2
  cat "$WORKDIR/run1.out" >&2
  echo "---- run2 ----" >&2
  cat "$WORKDIR/run2.out" >&2
  exit 1
fi

if ! grep -q "Front brake pad" "$WORKDIR/run2.out"; then
  echo "smoke_persist: description did not reload" >&2
  cat "$WORKDIR/run2.out" >&2
  exit 1
fi

if ! grep -q "Stock: 10" "$WORKDIR/run2.out"; then
  echo "smoke_persist: stock did not reload as 10" >&2
  cat "$WORKDIR/run2.out" >&2
  exit 1
fi

echo "smoke_persist: OK (add → quit → restart → search)"
