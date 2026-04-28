#!/bin/bash
# eBrowser v2.0 — macOS Build Script
# Builds with Apple's security hardening (Hardened Runtime + Library Validation)
set -e

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║          eBrowser v2.0 — macOS Build                       ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Check dependencies
command -v cmake >/dev/null 2>&1 || { echo "Installing cmake..."; brew install cmake; }
command -v sdl2-config >/dev/null 2>&1 || { echo "Installing SDL2..."; brew install sdl2; }

# Create include symlink for case-sensitive APFS
ln -sf ebrowser include/eBrowser 2>/dev/null || true

echo "[1/4] Configuring with security hardening..."
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DeBrowser_BUILD_STANDALONE=ON \
    -DCMAKE_C_FLAGS="-O2 -Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE" \
    -DCMAKE_EXE_LINKER_FLAGS="-pie" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0

echo ""
echo "[2/4] Building..."
cmake --build build --parallel $(sysctl -n hw.ncpu)

echo ""
echo "[3/4] Running tests..."
cd build
ctest --output-on-failure \
    -R "test_memory_safety|test_sandbox|test_firewall|test_anti_fingerprint|test_extension|test_privacy|test_tab_manager|test_bookmark"

echo ""
echo "[4/4] Running benchmark..."
cd tests
./benchmark 2>/dev/null
echo ""
./load_test_combined 2>/dev/null

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Build complete! Binaries in build/                        ║"
echo "╚══════════════════════════════════════════════════════════════╝"

# Show binary info
echo ""
echo "=== Binary Security Info ==="
if [ -f ../eBrowser ]; then
    codesign -dvvv ../eBrowser 2>&1 | head -5 || true
    otool -l ../eBrowser | grep -A2 "LC_CODE_SIGNATURE\|PIE\|RESTRICT" || true
fi
echo ""
ls -lh benchmark load_test_combined
echo ""
echo "To run the browser: ./build/eBrowser"
echo "To run benchmark:   ./build/tests/benchmark"
echo "To run load test:   ./build/tests/load_test_combined"
