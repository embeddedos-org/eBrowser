@echo off
:: eBrowser v2.0 — Windows Build Script
echo ============================================
echo   eBrowser v2.0 — Windows Build
echo ============================================
echo.

:: Check for cmake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: cmake not found. Install from https://cmake.org
    exit /b 1
)

:: Check for vcpkg and install SDL2 if available
set "HAS_SDL2=0"
where vcpkg >nul 2>&1
if not errorlevel 1 (
    echo [1/5] Installing SDL2 via vcpkg...
    vcpkg install sdl2:x64-windows
    set "HAS_SDL2=1"
    set "VCPKG_CMAKE=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
) else if defined VCPKG_INSTALLATION_ROOT (
    echo [1/5] Installing SDL2 via vcpkg...
    "%VCPKG_INSTALLATION_ROOT%\vcpkg.exe" install sdl2:x64-windows
    set "HAS_SDL2=1"
    set "VCPKG_CMAKE=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALLATION_ROOT%\scripts\buildsystems\vcpkg.cmake"
) else (
    echo [1/5] vcpkg not found — building without standalone GUI.
    echo       To build with GUI, install vcpkg: https://vcpkg.io
)

:: Create include alias (copy for Windows)
if not exist "include\eBrowser" xcopy /E /I /Q "include\ebrowser" "include\eBrowser" >nul

:: Configure
echo.
if "%HAS_SDL2%"=="1" (
    echo [2/5] Configuring with full browser support...
    cmake -B build %VCPKG_CMAKE% ^
        -DBUILD_TESTING=ON ^
        -DeBrowser_BUILD_STANDALONE=ON ^
        -DeBrowser_BUILD_BROWSER=ON ^
        -DeBrowser_BUILD_RENDER=ON
) else (
    echo [2/5] Configuring (libraries + tests only)...
    cmake -B build -DBUILD_TESTING=ON ^
        -DeBrowser_BUILD_STANDALONE=OFF ^
        -DeBrowser_BUILD_BROWSER=OFF ^
        -DeBrowser_BUILD_RENDER=OFF
)
if errorlevel 1 exit /b 1

:: Build
echo.
echo [3/5] Building...
cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1

:: Test
echo.
echo [4/5] Running tests...
cd build
ctest --output-on-failure -C Release ^
    -R "test_memory_safety|test_sandbox|test_firewall|test_anti_fingerprint|test_extension|test_privacy|test_tab_manager|test_bookmark"
cd ..

:: Launch
echo.
echo ============================================
echo   Build complete!
echo ============================================
echo.

if "%HAS_SDL2%"=="1" (
    if exist "build\port\Release\eBrowser.exe" (
        echo [5/5] Launching eBrowser...
        start "" "build\port\Release\eBrowser.exe" %1
    ) else if exist "build\port\eBrowser.exe" (
        echo [5/5] Launching eBrowser...
        start "" "build\port\eBrowser.exe" %1
    ) else (
        echo Standalone binary not found. Check build output above.
        echo Run benchmark:  build\tests\Release\benchmark.exe
    )
) else (
    echo No standalone browser (SDL2 not available).
    echo Run benchmark:  build\tests\Release\benchmark.exe
    echo Run load test:  build\tests\Release\load_test_combined.exe
)
