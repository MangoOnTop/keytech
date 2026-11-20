@echo off
if not exist build mkdir build

echo [1/2] Compiling KEYTECH (Professional UI + In-Game Overlay)...
g++ -std=c++17 -O2 ^
    -I third_party/ViGEmClient/include ^
    -I third_party/json ^
    src/main.cpp ^
    src/mapping.cpp ^
    src/ui.cpp ^
    third_party/ViGEmClient/src/ViGEmClient.cpp ^
    -o build/keytech.exe ^
    -lgdiplus ^
    -lgdi32 ^
    -luser32 ^
    -ldwmapi ^
    -lcomctl32 ^
    -lwinmm ^
    -lsetupapi ^
    -static-libgcc ^
    -static-libstdc++ ^
    -static ^
    -mwindows

if errorlevel 1 (
    echo.
    echo Compilation failed! Please check error messages above.
    exit /b 1
)

echo [2/2] Verifying static binary dependencies...
objdump -p build\keytech.exe | findstr "libgcc libstdc libwinpthread"
if errorlevel 1 (
    echo OK - Binary is 100%% self-contained. No external runtime DLLs required!
) else (
    echo Note: Some MinGW DLLs were linked dynamically.
)

echo.
echo ========================================================
echo   KEYTECH Build Succeeded!
echo   Run: build\keytech.exe
echo ========================================================
