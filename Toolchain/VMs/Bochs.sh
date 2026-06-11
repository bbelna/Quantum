#!/bin/bash
# Launch Bochs with Quantum OS images (floppy or HDD, auto-detected).
#
# Usage: Bochs.sh <primary.img> [extra.img]

set -euo pipefail

IMG="${1:?Usage: Bochs.sh <primary.img> [extra.img]}"
IMG2="${2:-}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

KERNEL_ELF="$REPO_ROOT/Build/Kernel/Kernel.qbn.elf"
LOG_DIR="$SCRIPT_DIR/Logs"
BOCHS_LOG="$LOG_DIR/Bochs.log"
SERIAL_LOG="$LOG_DIR/Bochs-Serial.log"

mkdir -p "$LOG_DIR"
> "$BOCHS_LOG"
> "$SERIAL_LOG"

# Detect media type: floppy images are <= 2 MB, HDD images are larger
IMG_SIZE=$(stat -c%s "$IMG")
IS_HDD=false
if [ "$IMG_SIZE" -gt 2097152 ]; then
  IS_HDD=true
fi

# Patch .bochsrc with the provided image paths via a temp copy
BOCHSRC_TEMP=$(mktemp)
trap "rm -f $BOCHSRC_TEMP" EXIT

if [ "$IS_HDD" = true ]; then
  # HDD mode: compute CHS geometry for the superfloppy image
  IMG_SECTORS=$(( IMG_SIZE / 512 ))
  HEADS=16
  SPT=63
  CYLINDERS=$(( IMG_SECTORS / (HEADS * SPT) ))

  # Remove floppy lines, switch boot device, and add ATA disk
  sed \
    -e "s|^boot:.*|boot: disk|" \
    -e "/^floppya:/d" \
    -e "/^floppyb:/d" \
    -e "s|^com1:.*|com1: enabled=1, mode=file, dev=$SERIAL_LOG|" \
    -e "s|^log:.*|log: $BOCHS_LOG|" \
    "$SCRIPT_DIR/.bochsrc" > "$BOCHSRC_TEMP"

  echo "ata0-master: type=disk, path=$IMG, mode=flat, cylinders=$CYLINDERS, heads=$HEADS, spt=$SPT" >> "$BOCHSRC_TEMP"
else
  sed \
    -e "s|^floppya:.*|floppya: 1_44=$IMG, status=inserted|" \
    -e "s|^floppyb:.*|floppyb: 1_44=$IMG2, status=inserted|" \
    -e "s|^com1:.*|com1: enabled=1, mode=file, dev=$SERIAL_LOG|" \
    -e "s|^log:.*|log: $BOCHS_LOG|" \
    "$SCRIPT_DIR/.bochsrc" > "$BOCHSRC_TEMP"
fi

echo "Launching Bochs ($( [ "$IS_HDD" = true ] && echo "HDD" || echo "Floppy" ))"
bochs -f "$BOCHSRC_TEMP" -q
echo "Bochs exited"

# ── Post-mortem ──────────────────────────────────────────────────────────────

echo ""
echo "=== Bochs-Serial.log (last 30 lines) ==="
tail -30 "$SERIAL_LOG"

echo ""
echo "=== Bochs.log (errors/panics) ==="
grep -E "\[CPU|panic|triple|fault|exception|error" "$BOCHS_LOG" | tail -30

if [ -f "$KERNEL_ELF" ]; then
  echo ""
  echo "=== Symbol resolution ==="
  grep -oE '0x[Cc]0[0-9a-fA-F]{6}' "$BOCHS_LOG" | sort -u | while read addr; do
    result=$(addr2line -e "$KERNEL_ELF" -f -C "$addr" 2>/dev/null)
    if [ -n "$result" ] && ! echo "$result" | grep -q "^??"; then
      echo "  $addr => $result"
    fi
  done
fi
