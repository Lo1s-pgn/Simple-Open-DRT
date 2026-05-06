#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(sed -n '1p' "${ROOT_DIR}/VERSION" | awk '{print $1}')"
PLUGIN_STEM="LSP_Simple_Open_DRT_${VERSION}"
BUNDLE_PATH="${ROOT_DIR}/release/${PLUGIN_STEM}.ofx.bundle"
DEST_DIR="/Library/OFX/Plugins"

if [ ! -d "${BUNDLE_PATH}" ]; then
  echo "Bundle not found: ${BUNDLE_PATH}"
  exit 1
fi

sudo mkdir -p "${DEST_DIR}"
sudo rm -rf "${DEST_DIR}/${PLUGIN_STEM}.ofx.bundle"
sudo cp -R "${BUNDLE_PATH}" "${DEST_DIR}/"
echo "Installed to ${DEST_DIR}/${PLUGIN_STEM}.ofx.bundle"
