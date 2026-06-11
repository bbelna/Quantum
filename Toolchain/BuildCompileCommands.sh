#!/usr/bin/env bash
# Generate compile_commands.json by wrapping the real Quantum build with Bear.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_SCRIPT="$REPO_ROOT/Build.sh"
OUTPUT="$REPO_ROOT/compile_commands.json"

if ! command -v bear &>/dev/null; then
  echo "Error: bear is not installed."
  echo "  Arch:   sudo pacman -S bear"
  echo "  Debian: sudo apt install bear"
  echo "  Fedora: sudo dnf install bear"
  exit 1
fi

echo ">>> Generating compile_commands.json via Bear"
echo "    Build script: $BUILD_SCRIPT"
echo "    Output:       $OUTPUT"

bear --output "$OUTPUT" -- bash "$BUILD_SCRIPT" --clean --skip-doxygen "$@"

echo ">>> Done, $(wc -l < "$OUTPUT") lines written to $OUTPUT"
