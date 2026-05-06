#!/bin/bash
set -euo pipefail

USER_HOME="${HOME}"
if [ -n "${SUDO_USER:-}" ]; then
  USER_HOME="$(dscl . -read "/Users/${SUDO_USER}" NFSHomeDirectory | awk '{print $2}')"
fi

rm -f "${USER_HOME}/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml"
rm -f "${USER_HOME}/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCache.xml"
echo "Resolve OFX cache purged for ${USER_HOME}"
