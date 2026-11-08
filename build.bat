@echo off
if not exist build mkdir build

echo [1/2] Compiling...
g++ -std=c++17 -I third_party/ViGEmClient/include -I third_party/json src/main.cpp src/mapping.cpp third_party/ViGEmClient/src/ViGEmClient.cpp -o build/keytech.exe -lsetupapi
if errorlevel 1 (
    echo Compilation failed.
    exit /b 1
)
echo Compilation OK.

echo [2/2] Signing (a UAC prompt will appear - click Yes)...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process powershell -Verb RunAs -Wait -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""%~dp0sign.ps1"" ""%~dp0build\keytech.exe""'"

echo.
echo Done. Run build\keytech.exe
