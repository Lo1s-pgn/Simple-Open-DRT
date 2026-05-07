@echo off
setlocal

REM Repo root: .../source/scripts/windows -> ../../../
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\build"
set "CMAKE_EXE=cmake"
for /f "tokens=1" %%V in (%ROOT%\VERSION) do set "VERSION=%%V"
set "PLUGIN_STEM=LSP_Simple_Open_DRT_%VERSION%"

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44 >nul 2>nul
if errorlevel 1 (
  call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" || exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "NVCC_PREPEND_FLAGS=-allow-unsupported-compiler"

"%CMAKE_EXE%" -S "%ROOT%\source" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release || exit /b 1
"%CMAKE_EXE%" --build "%BUILD_DIR%" || exit /b 1

echo.
echo Built plugin:
echo   %ROOT%\release\%PLUGIN_STEM%.ofx.bundle\Contents\Win64\%PLUGIN_STEM%.ofx
