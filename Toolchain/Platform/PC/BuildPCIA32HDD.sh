#!/bin/bash
# BuildPCIA32HDD.sh
# Assemble the Quantum HDD image for PC/IA32.
#
# Usage: BuildPCIA32HDD.sh [--version=DR0] [--variant=Alpha]
#
# Expects all binaries to be already built (run Build.sh first).
# Creates a 32 MB FAT12 hard disk image with the full graphical OS:
# kernel, servers, drivers, apps, fonts, cursors, and commands.
#
# Output: Build/Platform/PC/Quantum-{VERSION}-PC-IA32.img

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
IMG="$OUTPUT_DIR/Quantum-$RELEASE-PC-IA32-HDD.img"

# 32 MB disk image
IMG_SECTORS=65536
IMG_BS=512
FAT12_LABEL="QUANTUM"

# --- Platform config ---

PLATFORM="PC"
ARCH="IA32"
MEDIA="HDD"

BUNDLER="$REPO_ROOT/Toolchain/StartupBundle/CreateStartupBundle.py"
STARTUP_BUNDLE="$OUTPUT_DIR/Startup.qbd"

# --- Binary paths ---

BUILD_ROOT="$REPO_ROOT/Build"
BOOTLOADER_DIR="$BUILD_ROOT/Bootloader/IA32/PC/HDD"
FLOPPY_BOOTLOADER_DIR="$BUILD_ROOT/Bootloader/IA32/PC/Floppy"
STAGE1="$BOOTLOADER_DIR/Stage1.bin"
STAGE2="$BOOTLOADER_DIR/Stage2.bin"
BOOT="$FLOPPY_BOOTLOADER_DIR/Boot.qbn"
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
ABOUT_APP="$BUILD_ROOT/Apps/About/About.qapp"
ECHO_CMD="$BUILD_ROOT/Utilities/echo/echo.qx"
CWD_CMD="$BUILD_ROOT/Utilities/cwd/cwd.qx"
LS_CMD="$BUILD_ROOT/Utilities/ls/ls.qx"
VOL_CMD="$BUILD_ROOT/Utilities/vols/vols.qx"
CAT_CMD="$BUILD_ROOT/Utilities/cat/cat.qx"
PS_CMD="$BUILD_ROOT/Utilities/ps/ps.qx"
MEM_CMD="$BUILD_ROOT/Utilities/mem/mem.qx"
KMEM_CMD="$BUILD_ROOT/Utilities/kmem/kmem.qx"
DEV_CMD="$BUILD_ROOT/Utilities/devstat/DeviceServer.qx"
VER_CMD="$BUILD_ROOT/Utilities/ver/ver.qx"
SCHSTAT_CMD="$BUILD_ROOT/Utilities/schstat/schstat.qx"
QSH="$BUILD_ROOT/Utilities/qsh/qsh.qx"
UI_SHOWCASE="$BUILD_ROOT/Apps/UIShowcase/UIShowcase.qapp"

# Assets
HELLO="$REPO_ROOT/Assets/Hello.msg"
CREDITS="$REPO_ROOT/Assets/Credits.txt"
LICENSE="$REPO_ROOT/Assets/License.md"
DEFAULT_FONT="$REPO_ROOT/Toolchain/Fonts/Assets/QBF/dejavu-regular-12.qbf"
TITLE_FONT="$REPO_ROOT/Toolchain/Fonts/Assets/QBF/dejavu-regular-12.qbf"
MONO_FONT="$REPO_ROOT/Toolchain/Fonts/Assets/PSF/spleen-8x16.psf"
TERMINUS_FONT="$REPO_ROOT/Toolchain/Fonts/Assets/PSF/terminus-8x14.psf"
SYSTEM_CURSORS="$REPO_ROOT/Toolchain/Cursors/Assets/system-cursors.qcur"

# --- Helpers ---

copy_if_exists() {
  local source="$1" destination="$2"
  if [ -f "$source" ]; then
    mcopy -i "$IMG" "$source" "::$destination"
  fi
}

# --- Build ---

echo ">>> BuildPCIA32HDD: Quantum $RELEASE (PC/IA32) <<<"

mkdir -p "$OUTPUT_DIR"

# Startup bundle
echo "  BUNDLE $STARTUP_BUNDLE"
python3 "$BUNDLER" \
  --platform "$PLATFORM" \
  --arch "$ARCH" \
  --media "$MEDIA" \
  --output "$STARTUP_BUNDLE" \
  --base "$REPO_ROOT"

# --- HDD image (32 MB FAT12, no partition table) ---
#
# We create a "superfloppy" layout: the FAT12 file system starts at
# sector 0 with the VBR (no MBR, no partition table). This is the
# simplest layout and works well with QEMU -hda. The VBR has
# HiddenSectors=0 since there is no partition offset.

echo "  DD     $IMG (blank ${IMG_SECTORS} sectors)"
dd if=/dev/zero of="$IMG" bs=$IMG_BS count=$IMG_SECTORS conv=notrunc status=none

echo "  MKFS   $IMG (FAT12, label $FAT12_LABEL)"
mkfs.vfat -F 12 -R 3 -S 512 -n "$FAT12_LABEL" "$IMG" > /dev/null

echo "  MCOPY  Boot.qbn"
mcopy -i "$IMG" "$BOOT" ::/Boot.qbn

mmd -i "$IMG" \
  ::/System ::/System/Servers ::/System/Drivers \
  ::/System/Fonts ::/System/Cursors ::/System/Apps \
  ::/Apps ::/Binaries

echo "  MCOPY  Kernel.qbn"
mcopy -i "$IMG" "$KERNEL" ::/System/Kernel.qbn

# Write VBR boot code into sector 0, preserving the BPB (bytes 3-61)
# that mkfs.vfat wrote. We splice in:
#   - bytes 0-2   (jump instruction)
#   - bytes 62-509 (boot code after extended BPB)
echo "  DD     Stage1.bin -> sector 0 (VBR, preserving BPB)"
dd if="$STAGE1" of="$IMG" bs=1 count=3 conv=notrunc status=none
dd if="$STAGE1" of="$IMG" bs=1 skip=62 seek=62 count=448 conv=notrunc status=none

echo "  DD     Stage2.bin -> sectors 1-2"
dd if="$STAGE2" of="$IMG" bs=$IMG_BS seek=1 count=2 conv=notrunc,sync status=none

# --- Copy files ---

copy_if_exists "$STARTUP_BUNDLE"  "/System/Startup.qbd"
copy_if_exists "$HELLO"           "/System/Hello.msg"
copy_if_exists "$CREDITS"         "/System/Credits.txt"
copy_if_exists "$LICENSE"         "/System/License.md"

# Servers
copy_if_exists "$GRAPHICS"        "/System/Servers/GraphicsServer.qx"
copy_if_exists "$INPUT"           "/System/Servers/InputServer.qx"
copy_if_exists "$APP_SERVER"      "/System/Servers/AppServer.qx"
copy_if_exists "$CONTEXT_SERVER"  "/System/Servers/ContextServer.qx"

# Drivers
copy_if_exists "$PS2KEYBOARD"     "/System/Drivers/PS2Keyboard.qx"
copy_if_exists "$PS2MOUSE"        "/System/Drivers/PS2Mouse.qx"

# Fonts
copy_if_exists "$DEFAULT_FONT"    "/System/Fonts/Default.qbf"
copy_if_exists "$TITLE_FONT"      "/System/Fonts/TitleBar.qbf"
copy_if_exists "$MONO_FONT"       "/System/Fonts/Mono.psf"
copy_if_exists "$TERMINUS_FONT"   "/System/Fonts/Terminus12.psf"

# Cursors
copy_if_exists "$SYSTEM_CURSORS"  "/System/Cursors/System.qcur"

# Utilities (text mode) + commands
copy_if_exists "$QSH"       "/Binaries/qsh.qx"
copy_if_exists "$ECHO_CMD"       "/Binaries/echo.qx"
copy_if_exists "$CWD_CMD"        "/Binaries/cwd.qx"
copy_if_exists "$LS_CMD"         "/Binaries/ls.qx"
copy_if_exists "$VOL_CMD"        "/Binaries/vols.qx"
copy_if_exists "$CAT_CMD"        "/Binaries/cat.qx"
copy_if_exists "$PS_CMD"         "/Binaries/ps.qx"
copy_if_exists "$MEM_CMD"        "/Binaries/mem.qx"
copy_if_exists "$KMEM_CMD"       "/Binaries/kmem.qx"
copy_if_exists "$DEV_CMD"        "/Binaries/devstat.qx"
copy_if_exists "$VER_CMD"        "/Binaries/ver.qx"
copy_if_exists "$SCHSTAT_CMD"    "/Binaries/schstat.qx"

# Utilities configuration (read by Utilities/qsh at startup; if PATH is absent
# the Utilities defaults to its own program directory)
TERMINAL_CFG="$OUTPUT_DIR/qsh.cfg"
echo "  GEN    qsh.cfg"
cat > "$TERMINAL_CFG" <<'EOF'
# QuantumOS Utilities configuration
EOF
mcopy -i "$IMG" "$TERMINAL_CFG" ::/Binaries/qsh.cfg

# Apps
copy_if_exists "$TERMINAL_APP"       "/Apps/Terminal.qapp"
copy_if_exists "$ABOUT_APP"       "/Apps/About.qapp"
copy_if_exists "$UI_SHOWCASE"      "/Apps/UIShowcase.qapp"

echo ">>> BuildPCIA32HDD: OK <<<"
echo "  Image: $IMG"
