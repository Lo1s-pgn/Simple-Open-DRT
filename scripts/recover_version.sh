#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
VERSION_FILE="${ROOT_DIR}/VERSION"

if [ ! -f "${VERSION_FILE}" ]; then
  echo "Missing VERSION file"
  exit 1
fi

echo "Current version: $(sed -n '1p' "${VERSION_FILE}")"
