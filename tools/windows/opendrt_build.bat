@echo off
REM LSP Simple Open DRT — Windows build (Ninja + MSVC + CUDA).
REM Output: release\LSP_Simple_Open_DRT_<version>_windows\LSP_Simple_Open_DRT_<version>.ofx.bundle
REM Usage: tools\windows\opendrt_build.bat
REM        tools\windows\opendrt_build.bat clean

setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH (
  echo Visual Studio with MSVC x64 tools not found
  exit /b 1
)
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1

cd /d "%~dp0..\..\"

set "NVCC_PREPEND_FLAGS=-allow-unsupported-compiler"

cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
if errorlevel 1 exit /b 1

if /i "%~1"=="clean" (
  cmake --build build/windows --target clean
  if errorlevel 1 exit /b 1
)

cmake --build build/windows --target opendrt_all --parallel
exit /b %ERRORLEVEL%
