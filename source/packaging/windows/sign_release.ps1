# sign_release.ps1
$ErrorActionPreference = "Stop"

$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$version = Get-Content (Join-Path $repoRoot "VERSION") | Select-Object -First 1
$version = $version.Trim().Split(" ")[0]
$stem = "LSP_Simple_Open_DRT_$version"
$plugin = Join-Path $repoRoot "release\$stem.ofx.bundle\Contents\Win64\$stem.ofx"
$installer = Join-Path $PSScriptRoot "LSP_Simple_Open_DRT_v$version`_Windows_cuda_opencl_Installer.exe"
$pfx = "$env:USERPROFILE\Desktop\LSP_Simple_Open_DRT_SelfSign.pfx"

if (!(Test-Path $signtool)) { throw "signtool not found: $signtool" }
if (!(Test-Path $plugin)) { throw "plugin not found: $plugin" }
if (!(Test-Path $installer)) {
  Write-Host "installer not found: $installer"
  Write-Host "Continuing with plugin-only signing."
  $installer = $null
}

if (!(Test-Path $pfx)) {
  Write-Host "No PFX found. Creating self-signed code-signing cert..."
  $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=Lois Plagnard" -CertStoreLocation "Cert:\CurrentUser\My" -HashAlgorithm SHA256
  $pwSecure = Read-Host "Set password for new PFX" -AsSecureString
  Export-PfxCertificate -Cert $cert -FilePath $pfx -Password $pwSecure | Out-Null
  Write-Host "Created: $pfx"
}

$pwSecure2 = Read-Host "Enter PFX password" -AsSecureString
$pwPlain = [Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($pwSecure2))

Write-Host "Signing plugin..."
& $signtool sign /fd SHA256 /f $pfx /p $pwPlain /tr http://timestamp.digicert.com /td SHA256 $plugin

if ($installer) {
  Write-Host "Signing installer..."
  & $signtool sign /fd SHA256 /f $pfx /p $pwPlain /tr http://timestamp.digicert.com /td SHA256 $installer
}

Write-Host "Verifying plugin..."
& $signtool verify /pa /v $plugin

if ($installer) {
  Write-Host "Verifying installer..."
  & $signtool verify /pa /v $installer
}

Write-Host "`nDone."
