@echo off

where g++ >nul 2>nul
if errorlevel 1 (
    echo.
    echo ERROR: g++ ^(C++ compiler^) not found on PATH.
    echo.
    echo To install it:
    echo   1. Download MSYS2 from https://www.msys2.org and install it
    echo   2. Open "MSYS2 MinGW64" terminal ^(from Start Menu^)
    echo   3. Run: pacman -S mingw-w64-x86_64-gcc
    echo   4. Add C:\msys64\mingw64\bin to your system PATH
    echo      ^(System Properties ^> Environment Variables ^> Path ^> New^)
    echo   5. Close and reopen this terminal, then run this script again
    echo.
    exit /b 1
)

if not exist build\keytech.exe (
    echo First run: building KEYTECH...
    call build.bat
    if errorlevel 1 (
        echo.
        echo ERROR: Build failed. See errors above.
        exit /b 1
    )
)

echo Launching KEYTECH...
start "" "build\keytech.exe"
