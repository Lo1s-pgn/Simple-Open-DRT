# Backward-compatible wrapper.
# Canonical script now lives in source/packaging/windows/.
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$canonical = Join-Path $scriptRoot "packaging\windows\sign_release.ps1"
if (!(Test-Path $canonical)) {
  throw "Canonical signing script not found: $canonical"
}
& $canonical
