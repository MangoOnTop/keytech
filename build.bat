@echo off
if not exist build mkdir build

echo [1/2] Compiling (static linking - no external DLLs needed)...
g++ -std=c++17 ^
    -I third_party/ViGEmClient/include ^
    -I third_party/json ^
    src/main.cpp ^
    src/mapping.cpp ^
    third_party/ViGEmClient/src/ViGEmClient.cpp ^
    -o build/keytech.exe ^
    -lsetupapi ^
    -static-libgcc ^
    -static-libstdc++ ^
    -static

if errorlevel 1 (
    echo Compilation failed.
    exit /b 1
)

echo [2/2] Verifying no MinGW runtime dependencies...
objdump -p build\keytech.exe | findstr "libgcc libstdc libwinpthread"
if errorlevel 1 (
    echo OK - no MinGW DLL dependencies. Exe is fully self-contained.
) else (
    echo WARNING: some MinGW DLLs are still linked dynamically.
)

echo.
echo Done. Run build\keytech.exe
