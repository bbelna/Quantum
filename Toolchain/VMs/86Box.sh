#!/bin/bash
# Launch 86Box with the Quantum OS S3 ViRGE configuration (floppy or HDD).
#
# Usage: 86Box.sh <primary.img> [extra.img]

set -euo pipefail

IMG="${1:?Usage: 86Box.sh <primary.img> [extra.img]}"
IMG2="${2:-}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

KERNEL_ELF="$REPO_ROOT/Build/Kernel/Kernel.qbn.elf"
LOG_DIR="$SCRIPT_DIR/Logs"
SERIAL_LOG="$LOG_DIR/86Box-Serial.log"
STDERR_LOG="$LOG_DIR/86Box-stderr.log"

mkdir -p "$LOG_DIR"
> "$SERIAL_LOG"

# Detect media type: floppy images are <= 2 MB, HDD images are larger
IMG_SIZE=$(stat -c%s "$IMG")
IS_HDD=false
if [ "$IMG_SIZE" -gt 2097152 ]; then
  IS_HDD=true
fi

# ── Build patched 86box.conf ─────────────────────────────────────────────────
CONF_TEMP=$(mktemp)

cleanup() {
  kill "$CAT_PID" 2>/dev/null || true
  rm -f "$CONF_TEMP"
}
trap cleanup EXIT
CAT_PID=0

if [ "$IS_HDD" = true ]; then
  # HDD mode: clear floppy entries, add hard disk configuration
  IMG_SECTORS=$(( IMG_SIZE / 512 ))
  HEADS=16
  SPT=63
  CYLINDERS=$(( IMG_SECTORS / (HEADS * SPT) ))

  sed \
    -e "/^\[Floppy and CD-ROM drives\]/d" \
    -e "/^fdd_/d" \
    "$SCRIPT_DIR/86box.conf" > "$CONF_TEMP"

  # Append hard disk section (IDE primary master)
  # 86Box hdd_parameters format: SPT, HPC, CYLINDERS, WP, BUS_TYPE
  cat >> "$CONF_TEMP" <<EOF

[Hard disks]
hdd_01_parameters = $SPT, $HEADS, $CYLINDERS, 0, ide
hdd_01_ide_channel = 0:0
hdd_01_fn = $IMG
EOF
else
  sed \
    -e "s|^fdd_01_fn = .*|fdd_01_fn = $IMG|" \
    -e "s|^fdd_02_fn = .*|fdd_02_fn = $IMG2|" \
    -e "s|^fdd_02_image_history_01 = .*|fdd_02_image_history_01 = $IMG2|" \
    "$SCRIPT_DIR/86box.conf" > "$CONF_TEMP"
fi

# Enable serial passthrough in vcon (PTY) mode. 86Box creates a PTY pair
# internally and prints the slave path to stderr. We parse that path and
# read from it to capture serial output.
cat >> "$CONF_TEMP" <<EOF

[Ports (COM & LPT)]
serial1_enabled = 1
serial1_passthrough_enabled = 1

[Serial Passthrough Device #1]
mode = 0
baudrate = 115200
data_bits = 8
stop_bits = 1
EOF

echo "Launching 86Box (S3 ViRGE, $( [ "$IS_HDD" = true ] && echo "HDD" || echo "Floppy" ))"
cd "$SCRIPT_DIR"

# Run 86Box in background so we can parse its stderr for the PTY path
86Box -C "$CONF_TEMP" 2>"$STDERR_LOG" &
BOX_PID=$!

# Wait for 86Box to print the slave PTY path, then start capturing
PTY_PATH=""
for i in $(seq 1 40); do
  PTY_PATH=$(grep -oP '/dev/pts/\d+' "$STDERR_LOG" 2>/dev/null | head -1) || true
  [ -n "$PTY_PATH" ] && break
  sleep 0.1
done

if [ -n "$PTY_PATH" ]; then
  cat "$PTY_PATH" > "$SERIAL_LOG" &
  CAT_PID=$!
  echo "  Serial logging: $PTY_PATH -> $SERIAL_LOG"
else
  echo "  Warning: could not detect serial PTY, serial logging disabled"
fi

wait "$BOX_PID" 2>/dev/null || true
rm -rf ./nvr/
echo "86Box exited"

# ── Post-mortem ──────────────────────────────────────────────────────────────

echo ""
echo "=== 86Box-Serial.log (last 30 lines) ==="
tail -30 "$SERIAL_LOG"

if [ -f "$KERNEL_ELF" ]; then
  echo ""
  echo "=== Symbol resolution ==="
  grep -oE '0x[Cc]0[0-9a-fA-F]{6}' "$SERIAL_LOG" | sort -u | while read addr; do
    result=$(addr2line -e "$KERNEL_ELF" -f -C "$addr" 2>/dev/null)
    if [ -n "$result" ] && ! echo "$result" | grep -q "^??"; then
      echo "  $addr => $result"
    fi
  done
fi
