#!/usr/bin/env bash
# Run clang-tidy on specified paths, or on changed files if none given.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPDB="$REPO_ROOT/compile_commands.json"

if ! command -v clang-tidy &>/dev/null; then
  echo "Error: clang-tidy is not installed."
  exit 1
fi

if [ ! -f "$COMPDB" ]; then
  echo "Error: $COMPDB not found."
  echo "Run Tools/compdb.sh first to generate it."
  exit 1
fi

if [ $# -gt 0 ]; then
  # Tidy files matching the given path(s)
  FILES=()
  for arg in "$@"; do
    while IFS= read -r -d '' f; do
      FILES+=("$f")
    done < <(find "$arg" -type f \( -name '*.cpp' -o -name '*.c' \) \
              ! -path '*/Build/*' -print0)
  done
else
  # Tidy only files changed vs the current branch point
  mapfile -t FILES < <(
    git -C "$REPO_ROOT" diff --name-only --diff-filter=ACM HEAD \
      -- '*.cpp' '*.c' |
    sed "s|^|$REPO_ROOT/|"
  )
  if [ ${#FILES[@]} -eq 0 ]; then
    echo "No changed .cpp/.c files found."
    exit 0
  fi
fi

echo ">>> Running clang-tidy on ${#FILES[@]} file(s)"

clang-tidy -p "$COMPDB" "${FILES[@]}"

echo ">>> Done"
