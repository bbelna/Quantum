#!/bin/bash
# BuildPCIA32Floppy.sh
# Assemble the Quantum floppy image for PC/IA32.
#
# Usage: BuildPCIA32Floppy.sh [--version=DR0] [--variant=Alpha]
#
# Expects all binaries to be already built (run Build.sh first).
# Creates the startup bundle from the PC/IA32/Floppy manifest, then
# packs everything into a bootable 1.44 MB FAT12 floppy image.
#
# Output: Build/Platform/PC/Quantum-{VERSION}-PC-IA32.img         (boot+system)
#         Build/Platform/PC/Quantum-{VERSION}-PC-IA32-Binaries.img (commands)

set -euo pipefail

VERSION="DR0"
VARIANT=""

for arg in "$@"; do
  case $arg in
    --version=*) VERSION="${arg#*=}" ;;
    --variant=*) VARIANT="${arg#*=}" ;;
  esac
done

RELEASE="$VERSION$VARIANT"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# --- Output ---

OUTPUT_DIR="$REPO_ROOT/Build/Platform/PC"
IMG="$OUTPUT_DIR/Quantum-$RELEASE-PC-IA32.img"
IMG2="$OUTPUT_DIR/Quantum-$RELEASE-PC-IA32-Binaries.img"

IMG_SECTORS=2880
IMG_BS=512
FAT12_LABEL="QUANTUM"
FAT12_LABEL2="BINARIES"

# --- Platform config ---

PLATFORM="PC"
ARCH="IA32"
MEDIA="Floppy"

BUNDLER="$REPO_ROOT/Toolchain/StartupBundle/CreateStartupBundle.py"
STARTUP_BUNDLE="$OUTPUT_DIR/Startup.qbd"

# --- Binary paths ---

BUILD_ROOT="$REPO_ROOT/Build"
BOOTLOADER_DIR="$BUILD_ROOT/Bootloader/IA32/PC/Floppy"
STAGE1="$BOOTLOADER_DIR/Stage1.bin"
STAGE2="$BOOTLOADER_DIR/Stage2.bin"
BOOT="$BOOTLOADER_DIR/Boot.qbn"
KERNEL="$BUILD_ROOT/Kernel/Kernel.qbn"

# Servers
GRAPHICS="$BUILD_ROOT/Servers/Graphics/GraphicsServer.qx"
INPUT="$BUILD_ROOT/Servers/Input/InputServer.qx"
APP_SERVER="$BUILD_ROOT/Servers/App/AppServer.qx"
CONTEXT_SERVER="$BUILD_ROOT/Servers/Context/Content.qx"

# Drivers
PS2KEYBOARD="$BUILD_ROOT/Drivers/Input/PS2Keyboard/PS2Keyboard.qx"
PS2MOUSE="$BUILD_ROOT/Drivers/Input/PS2Mouse/PS2Mouse.qx"

# Apps & Commands
TERMINAL_APP="$BUILD_ROOT/Apps/Terminal/Terminal.qapp"
ABOUT_APP="$BUILD_ROOT/Apps/About.qapp"
ECHO_CMD="$BUILD_ROOT/Utilities/echo/echo.qx"
CWD_CMD="$BUILD_ROOT/Utilities/cwd/cwd.qx"
LS_CMD="$BUILD_ROOT/Utilities/ls/ls.qx"
VOL_CMD="$BUILD_ROOT/Utilities/vols/vols.qx"
CAT_CMD="$BUILD_ROOT/Utilities/cat/cat.qx"
PS_CMD="$BUILD_ROOT/Utilities/ps/ps.qx"
MEM_CMD="$BUILD_ROOT/Utilities/mem/mem.qx"
KMEM_CMD="$BUILD_ROOT/Utilities/kmem/kmem.qx"
DEV_CMD="$BUILD_ROOT/Utilities/devstat/devstat.qx"
VER_CMD="$BUILD_ROOT/Utilities/ver/ver.qx"
SCHSTAT_CMD="$BUILD_ROOT/Utilities/schstat/schstat.qx"
QSH="$BUILD_ROOT/Utilities/qsh/qsh.qx"
UI_SHOWCASE="$BUILD_ROOT/Apps/UIShowcase/UIShowcase.qapp"

# Assets
HELLO="$REPO_ROOT/Assets/Hello.msg"
CREDITS="$REPO_ROOT/Assets/Credits.txt"
LICENSE="$REPO_ROOT/Assets/License.md"
DEFAULT_FONT="$REPO_ROOT/Assets/Fonts/noto-sans-regular-12.qbf"
TITLE_FONT="$REPO_ROOT/Assets/Fonts/noto-sans-bold-12.qbf"
SYSTEM_CURSORS="$REPO_ROOT/Assets/Cursors/system-cursors.qcur"

# --- Helpers ---

copy_if_exists() {
  local image="$1" source="$2" destination="$3"
  if [ -f "$source" ]; then
    mcopy -i "$image" "$source" "::$destination"
  fi
}

# --- Build ---

echo ">>> BuildPCIA32Floppy: Quantum $RELEASE (PC/IA32) <<<"

mkdir -p "$OUTPUT_DIR"

# Startup bundle
echo "  BUNDLE $STARTUP_BUNDLE"
python3 "$BUNDLER" \
  --platform "$PLATFORM" \
  --arch "$ARCH" \
  --media "$MEDIA" \
  --output "$STARTUP_BUNDLE" \
  --base "$REPO_ROOT"

# --- Primary image (bootable 1.44 MB FAT12) ---

echo "  DD     $IMG (blank ${IMG_SECTORS} sectors)"
rm -f "$IMG"
dd if=/dev/zero of="$IMG" bs=$IMG_BS count=$IMG_SECTORS status=none

echo "  MKFS   $IMG (FAT12, label $FAT12_LABEL)"
mkfs.vfat -F 12 -R 3 -n "$FAT12_LABEL" "$IMG" > /dev/null

echo "  MCOPY  Boot.qbn"
mcopy -i "$IMG" "$BOOT" ::/Boot.qbn

mmd -i "$IMG" \
  ::/System ::/System/Servers ::/System/Drivers ::/Binaries

echo "  MCOPY  Kernel.qbn"
mcopy -i "$IMG" "$KERNEL" ::/System/Kernel.qbn

echo "  DD     Stage1.bin -> sector 0"
dd if="$STAGE1" of="$IMG" bs=$IMG_BS seek=0 count=1 conv=notrunc status=none

echo "  DD     Stage2.bin -> sectors 1-2"
dd if="$STAGE2" of="$IMG" bs=$IMG_BS seek=1 count=2 conv=notrunc,sync status=none

copy_if_exists "$IMG" "$STARTUP_BUNDLE"  "/System/Startup.qbd"
copy_if_exists "$IMG" "$HELLO"           "/System/Hello.msg"
copy_if_exists "$IMG" "$CREDITS"         "/System/Credits.txt"
copy_if_exists "$IMG" "$LICENSE"         "/System/License.md"
copy_if_exists "$IMG" "$GRAPHICS"        "/System/Servers/GraphicsServer.qx"
copy_if_exists "$IMG" "$INPUT"           "/System/Servers/InputServer.qx"
copy_if_exists "$IMG" "$PS2KEYBOARD"     "/System/Drivers/PS2Keyboard.qx"
copy_if_exists "$IMG" "$QSH"        "/Binaries/qsh.qx"

# --- Binaries floppy (commands) ---

echo "  DD     $IMG2 (blank ${IMG_SECTORS} sectors)"
dd if=/dev/zero of="$IMG2" bs=$IMG_BS count=$IMG_SECTORS conv=notrunc status=none

echo "  MKFS   $IMG2 (FAT12, label $FAT12_LABEL2)"
mkfs.vfat -F 12 -n "$FAT12_LABEL2" "$IMG2" > /dev/null

copy_if_exists "$IMG" "$LS_CMD"         "/Binaries/ls.qx"
copy_if_exists "$IMG" "$VOL_CMD"        "/Binaries/vols.qx"
copy_if_exists "$IMG" "$CWD_CMD"        "/Binaries/cwd.qx"
copy_if_exists "$IMG" "$CAT_CMD"        "/Binaries/cat.qx"
copy_if_exists "$IMG2" "$ECHO_CMD"       "/echo.qx"
copy_if_exists "$IMG2" "$PS_CMD"         "/ps.qx"
copy_if_exists "$IMG2" "$MEM_CMD"        "/mem.qx"
copy_if_exists "$IMG2" "$KMEM_CMD"       "/kmem.qx"
copy_if_exists "$IMG2" "$DEV_CMD"        "/devstat.qx"
copy_if_exists "$IMG2" "$VER_CMD"        "/ver.qx"
copy_if_exists "$IMG2" "$SCHSTAT_CMD"    "/schstat.qx"

echo ">>> BuildPCIA32Floppy: OK <<<"
echo "  Primary:  $IMG"
echo "  Binaries: $IMG2"
