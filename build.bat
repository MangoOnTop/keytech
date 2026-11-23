@echo off
setlocal enabledelayedexpansion

if not exist build mkdir build

echo ========================================================
echo   KEYTECH // COMPILATION ^& PACKAGING PIPELINE
echo ========================================================

echo [1/4] Compiling Windows Resources (App Icon + Manifest)...
windres resource.rc -O coff -o resource.o
if errorlevel 1 (
    echo Resource compilation failed!
    exit /b 1
)

echo [2/4] Compiling KEYTECH Engine (Realistic Gamepads + Pro UI)...
g++ -std=c++17 -O2 ^
    -I third_party/ViGEmClient/include ^
    -I third_party/json ^
    src/main.cpp ^
    src/mapping.cpp ^
    src/ui.cpp ^
    third_party/ViGEmClient/src/ViGEmClient.cpp ^
    resource.o ^
    -o build/keytech.exe ^
    -lgdiplus ^
    -lgdi32 ^
    -luser32 ^
    -ldwmapi ^
    -lcomctl32 ^
    -lwinmm ^
    -lsetupapi ^
    -lole32 ^
    -luuid ^
    -lurlmon ^
    -static-libgcc ^
    -static-libstdc++ ^
    -static ^
    -mwindows

if errorlevel 1 (
    echo Engine compilation failed!
    exit /b 1
)

echo [3/4] Compiling Installer Resources (Self-Extracting Payload)...
cd installer
windres installer.rc -O coff -o installer.res.o
if errorlevel 1 (
    echo Installer resource compilation failed!
    cd ..
    exit /b 1
)

echo [4/4] Building Standalone One-Click Installer (KeytechSetup.exe)...
g++ -std=c++17 -O2 ^
    installer.cpp ^
    installer.res.o ^
    -o ../build/KeytechSetup.exe ^
    -lgdiplus ^
    -lgdi32 ^
    -luser32 ^
    -lcomctl32 ^
    -lole32 ^
    -luuid ^
    -lurlmon ^
    -static-libgcc ^
    -static-libstdc++ ^
    -static ^
    -mwindows

cd ..

if errorlevel 1 (
    echo Installer compilation failed!
    exit /b 1
)

echo.
echo [Verify] Copying config.json to build folder...
copy /Y config.json build\config.json >nul
copy /Y icon.ico build\icon.ico >nul

echo.
echo ========================================================
echo   BUILD COMPLETE!
echo   1. Standalone Installer : build\KeytechSetup.exe
echo   2. Portable Engine      : build\keytech.exe
echo ========================================================
