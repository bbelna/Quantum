#!/bin/bash
# Debug.sh - Dispatcher that derives image paths from platform/arch/version
# and launches the requested emulator.
#
# Usage: Debug.sh <qemu|bochs|86box> <PLATFORM> <ARCH> [VERSION] [VARIANT]

set -euo pipefail

EMULATOR="${1:?Usage: Debug.sh <qemu|bochs|86box> <PLATFORM> <ARCH> [VERSION] [VARIANT]}"
PLATFORM="${2:?Usage: Debug.sh <qemu|bochs|86box> <PLATFORM> <ARCH> [VERSION] [VARIANT]}"
ARCH="${3:?Usage: Debug.sh <qemu|bochs|86box> <PLATFORM> <ARCH> [VERSION] [VARIANT]}"
VERSION="${4:-DR0}"
VARIANT="${5:-}"
MEDIA="${6:-Floppy}"

RELEASE="$VERSION$VARIANT"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Derive image paths from platform/arch/version/media
IMG_DIR="$REPO_ROOT/Build/Platform/$PLATFORM"

if [ "$MEDIA" = "HDD" ]; then
  IMG="$IMG_DIR/Quantum-$RELEASE-$PLATFORM-$ARCH-HDD.img"
else
  IMG="$IMG_DIR/Quantum-$RELEASE-$PLATFORM-$ARCH.img"
fi

IMG2="$IMG_DIR/Quantum-$RELEASE-$PLATFORM-$ARCH-Binaries.img"

if [ ! -f "$IMG" ]; then
  echo "Error: Primary image not found: $IMG"
  echo "  Run the platform image script first, or use Build.sh --qemu/--bochs/--86box."
  exit 1
fi

case "$EMULATOR" in
  qemu)  "$SCRIPT_DIR/QEMU.sh"  "$IMG" "$IMG2" ;;
  bochs) "$SCRIPT_DIR/Bochs.sh" "$IMG" "$IMG2" ;;
  86box) "$SCRIPT_DIR/86Box.sh" "$IMG" "$IMG2" ;;
  *)
    echo "Unknown emulator: $EMULATOR"
    echo "  Supported: qemu, bochs, 86box"
    exit 1
    ;;
esac
