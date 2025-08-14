#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
TARGET="${BUILD_DIR}/bookslice"
OUTPUT_DIRS=(chapters toc_sections chapter_segments)

clean_outputs() {
  for d in "${OUTPUT_DIRS[@]}"; do
    rm -rf "$d"
  done
}

echo "[run.sh] removing previous outputs (if any)"
clean_outputs

echo "[run.sh] cleaning previous build"
make clean

echo "[run.sh] building"
make build

# Pass all CLI args to the executable (robust quoting)
echo "[run.sh] running ${TARGET} with args: $*"
"${TARGET}" "$@"

# Optional: delete outputs after run (as requested)
echo "[run.sh] cleaning outputs after run"
clean_outputs

echo "[run.sh] done"
