#!/usr/bin/env bash
# Linux build helper — configure + build (release/LSP_Simple_Open_DRT_<version>_linux/).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build/linux --target opendrt_all --parallel "$(nproc 2>/dev/null || echo 4)"
