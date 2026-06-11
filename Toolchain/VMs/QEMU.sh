#!/bin/bash
# Launch QEMU with the Quantum OS floppy images.
#
# Usage: QEMU.sh <primary.img> [extra.img]

set -euo pipefail

IMG="${1:?Usage: QEMU.sh <primary.img> [extra.img]}"
IMG2="${2:-}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

KERNEL_ELF="$REPO_ROOT/Build/Kernel/Kernel.qbn.elf"
LOG_DIR="$SCRIPT_DIR/Logs"
SERIAL_LOG="$LOG_DIR/QEMU.log"

mkdir -p "$LOG_DIR"
> "$SERIAL_LOG"

# detect media type: floppy images are <= 2 MB, HDD images are larger
IMG_SIZE=$(stat -c%s "$IMG")
IS_HDD=false

if [ "$IMG_SIZE" -gt 2097152 ]; then
  IS_HDD=true
fi

DRIVE_ARGS=""

if [ "$IS_HDD" = true ]; then
  DRIVE_ARGS="-hda $IMG"
else
  DRIVE_ARGS="-fda $IMG"

  if [ -n "$IMG2" ] && [ -f "$IMG2" ]; then
    DRIVE_ARGS="$DRIVE_ARGS -fdb $IMG2"
  fi
fi

echo "Launching QEMU ($( [ "$IS_HDD" = true ] && echo "HDD" || echo "Floppy" ))"
GDK_BACKEND=x11 qemu-system-i386 \
  $DRIVE_ARGS \
  -m 32M \
  -monitor none \
  -serial file:"$SERIAL_LOG" \
  -parallel none \
  -no-reboot \
  -display sdl \
  -no-shutdown
echo "QEMU exited"

# ── Post-mortem ──────────────────────────────────────────────────────────────

echo ""
echo "=== QEMU.log (last 30 lines) ==="
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
