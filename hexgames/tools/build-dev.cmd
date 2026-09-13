@echo off
rem Copyright Ben Paul Wise. All Rights Reserved.
rem build-dev.cmd [preset] -- configure, build and test with the MSVC developer environment loaded.
rem Uses CLion's bundled Ninja when no ninja is on PATH. Default preset: win-msvc-debug.
setlocal
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=win-msvc-debug"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo build-dev: vcvars64.bat not found or failed
  exit /b 2
)
where ninja >nul 2>&1
if errorlevel 1 set "PATH=C:\Program Files\JetBrains\CLion 2026.1\bin\ninja\win\x64;%PATH%"
cd /d "%~dp0.."
cmake --preset %PRESET% || exit /b 1
cmake --build --preset %PRESET% || exit /b 1
ctest --preset %PRESET%
rem Copyright Ben Paul Wise. All Rights Reserved.
