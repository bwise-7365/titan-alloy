@echo off
rem Copyright Ben Paul Wise. All Rights Reserved.
rem build-dev.cmd [preset] -- configure, build and test with the MSVC developer environment loaded.
rem Uses CLion's bundled Ninja when no ninja is on PATH, and CLion's CMake/ctest when installed.
rem Default preset: win-msvc-debug.
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
rem One CMake for this build folder: CLion's when installed, so the command line and the IDE never
rem reconfigure the same cache with different CMake versions (the presets also need CMake 4.2 or later).
set "CLION_CMAKE=C:\Program Files\JetBrains\CLion 2026.1\bin\cmake\win\x64\bin"
if exist "%CLION_CMAKE%\cmake.exe" set "PATH=%CLION_CMAKE%;%PATH%"
cd /d "%~dp0.."
cmake --preset %PRESET% || exit /b 1
cmake --build --preset %PRESET% || exit /b 1
ctest --preset %PRESET%
rem Copyright Ben Paul Wise. All Rights Reserved.
