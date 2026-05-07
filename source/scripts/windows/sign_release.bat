@echo off
setlocal
set "PS1=%~dp0..\..\packaging\windows\sign_release.ps1"
if not exist "%PS1%" (
  echo Missing: "%PS1%"
  exit /b 1
)
powershell -ExecutionPolicy Bypass -File "%PS1%"
echo.
pause
