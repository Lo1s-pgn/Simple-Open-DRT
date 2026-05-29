#!/usr/bin/env bash
# macOS build helper — configure + build (release/LSP_Simple_Open_DRT_<version>_macos/).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
cmake -S . -B build/macos -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build/macos --target opendrt_all --parallel "$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
