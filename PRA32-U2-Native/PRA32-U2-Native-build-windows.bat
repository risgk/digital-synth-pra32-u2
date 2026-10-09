@echo off
rem Builds PRA32-U2 Native (VST3 and Standalone) with Visual Studio 2022 (x64, Release)
cd /d "%~dp0"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || exit /b 1
cmake --build build --config Release || exit /b 1
echo.
echo Outputs: build\PRA32_U2_Native_artefacts\Release\
