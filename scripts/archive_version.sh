#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="${ROOT_DIR}/VERSION"

if [[ ! -f "${VERSION_FILE}" ]]; then
  echo "VERSION file not found: ${VERSION_FILE}" >&2
  exit 1
fi

CURRENT_VERSION="$(tr -d '[:space:]' < "${VERSION_FILE}")"
IFS='.' read -r MAJOR MINOR PATCH <<< "${CURRENT_VERSION}"

if [[ -z "${MAJOR:-}" || -z "${MINOR:-}" || -z "${PATCH:-}" ]]; then
  echo "Invalid semantic version in VERSION: ${CURRENT_VERSION}" >&2
  exit 1
fi

NEXT_VERSION="${MAJOR}.${MINOR}.$((PATCH + 1))"
echo "${NEXT_VERSION}" > "${VERSION_FILE}"

export ROOT_DIR CURRENT_VERSION NEXT_VERSION
python3 - <<'PY'
from pathlib import Path
import os
import re

root = Path(os.environ["ROOT_DIR"])
current_version = os.environ["CURRENT_VERSION"]
next_version = os.environ["NEXT_VERSION"]

targets = [
    root / "source" / "CMakeLists.txt",
]

for path in targets:
    text = path.read_text()
    updated = text.replace(current_version, next_version)
    path.write_text(updated)

constants_h = root / "source" / "plugin" / "core" / "OpenDRTConstants.h"
const_text = constants_h.read_text()
const_text = re.sub(
    r'(#define kPluginVersionString\s+")([^"]+)(")',
    rf'\g<1>{next_version}\g<3>',
    const_text,
)
constants_h.write_text(const_text)

effect_cpp = root / "source" / "plugin" / "core" / "OpenDRTEffect.cpp"
eff_text = effect_cpp.read_text()
eff_text = re.sub(
    r'(nameWithVersion = "LSP - Simple Open DRT v)([^"]+)(")',
    rf'\g<1>{next_version}\g<3>',
    eff_text,
)
effect_cpp.write_text(eff_text)

readme = root / "README.md"
readme_text = readme.read_text().replace(
    f"Current version: `{current_version}`",
    f"Current version: `{next_version}`",
)
readme.write_text(readme_text)

presets_h = root / "source" / "plugin" / "core" / "OpenDRTPresets.h"
presets_text = presets_h.read_text()
presets_text = re.sub(
    r'(kOpenDRTPortVersion = ")([^"]+)(")',
    rf'\g<1>{next_version}\g<3>',
    presets_text,
)
presets_h.write_text(presets_text)

print(next_version)
PY

echo "Archived version: ${CURRENT_VERSION} -> ${NEXT_VERSION}"
