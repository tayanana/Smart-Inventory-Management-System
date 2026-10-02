#!/usr/bin/env bash
# Smoke test: administrator adds a part, save/quit, workshop user restarts and searches.
set -euo pipefail

BIN="${1:?path to AutoInventorySystem binary}"
WORKDIR="$(mktemp -d "${TMPDIR:-/tmp}/autoinv-smoke.XXXXXX")"
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

# 2 = administrator, 1 = add product, 11 = save and exit
"$BIN" "$WORKDIR" >"$WORKDIR/run1.out" <<'EOF'
2
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

# 1 = workshop user, 1 = search, 6 = exit (no rewrite)
"$BIN" "$WORKDIR" >"$WORKDIR/run2.out" <<'EOF'
1
1
BRAKE_PAD
6
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

if ! grep -q "workshop user" "$WORKDIR/run2.out"; then
  echo "smoke_persist: restart did not use the workshop-user client" >&2
  cat "$WORKDIR/run2.out" >&2
  exit 1
fi

echo "smoke_persist: OK (admin add → quit → user restart → search)"
