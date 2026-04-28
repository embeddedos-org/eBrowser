@echo off
:: eBrowser v2.0 — Windows Build Script (MSVC)
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

:: Create include alias (copy for Windows)
if not exist "include\eBrowser" xcopy /E /I /Q "include\ebrowser" "include\eBrowser" >nul

:: Configure
echo [1/3] Configuring...
cmake -B build -DBUILD_TESTING=ON ^
    -DeBrowser_BUILD_STANDALONE=OFF ^
    -DeBrowser_BUILD_BROWSER=OFF ^
    -DeBrowser_BUILD_RENDER=OFF
if errorlevel 1 exit /b 1

:: Build
echo.
echo [2/3] Building...
cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1

:: Test
echo.
echo [3/3] Running tests...
cd build
ctest --output-on-failure -C Release ^
    -R "test_memory_safety|test_sandbox|test_firewall|test_anti_fingerprint|test_extension|test_privacy|test_tab_manager|test_bookmark"

echo.
echo ============================================
echo   Build complete! Binaries in build\Release
echo ============================================
echo.
echo Run benchmark:  build\tests\Release\benchmark.exe
echo Run load test:  build\tests\Release\load_test_combined.exe
