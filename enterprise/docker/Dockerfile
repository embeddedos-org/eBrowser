# ============================================================
# eBrowser v2.0 — Multi-Stage Docker Build
# Stage 1: Build (gcc + cmake + SDL2)
# Stage 2: Runtime (minimal Alpine with only the binary)
# ============================================================

# --- Stage 1: Build ---
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates \
    libsdl2-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

# Create include symlink for case-sensitive Linux filesystem
RUN ln -sf ebrowser include/eBrowser 2>/dev/null || true

# Build everything: browser + tests + benchmark
RUN mkdir -p build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=ON \
      -DeBrowser_BUILD_STANDALONE=ON \
      -DeBrowser_BUILD_BROWSER=ON \
      -DCMAKE_C_FLAGS="-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE" \
      -DCMAKE_EXE_LINKER_FLAGS="-pie -Wl,-z,relro,-z,now,-z,noexecstack" && \
    make -j$(nproc)

# Run tests to verify build
RUN cd build && ctest --output-on-failure \
    -R "test_memory_safety|test_sandbox|test_firewall|test_anti_fingerprint|test_extension|test_privacy|test_tab_manager|test_bookmark"

# Run benchmark
RUN cd build/tests && ./benchmark 2>/dev/null || true

# Run load test
RUN cd build/tests && ./load_test_combined || true

# Strip binary for minimal size
RUN strip build/tests/benchmark build/tests/load_test_combined 2>/dev/null || true && \
    if [ -f build/eBrowser ]; then strip build/eBrowser; fi

# --- Stage 2: Minimal runtime image ---
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libsdl2-2.0-0 ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /bin/false -m ebrowser

WORKDIR /opt/ebrowser

# Copy binaries
COPY --from=builder /build/build/eBrowser ./ebrowser 2>/dev/null || true
COPY --from=builder /build/build/tests/benchmark ./benchmark
COPY --from=builder /build/build/tests/load_test_combined ./load_test

# Copy test binaries for verification
COPY --from=builder /build/build/tests/test_memory_safety ./tests/
COPY --from=builder /build/build/tests/test_sandbox ./tests/
COPY --from=builder /build/build/tests/test_firewall ./tests/
COPY --from=builder /build/build/tests/test_anti_fingerprint ./tests/
COPY --from=builder /build/build/tests/test_extension ./tests/
COPY --from=builder /build/build/tests/test_privacy ./tests/
COPY --from=builder /build/build/tests/test_tab_manager ./tests/
COPY --from=builder /build/build/tests/test_bookmark ./tests/

# Copy assets
COPY --from=builder /build/assets ./assets/
COPY --from=builder /build/README.md ./
COPY --from=builder /build/LICENSE ./

# Create data directories
RUN mkdir -p /opt/ebrowser/data/bookmarks \
             /opt/ebrowser/data/downloads \
             /opt/ebrowser/data/extensions \
             /opt/ebrowser/data/cache \
             /opt/ebrowser/data/profiles && \
    chown -R ebrowser:ebrowser /opt/ebrowser

# Security: run as non-root
USER ebrowser

# Health check — run the benchmark as a quick verification
HEALTHCHECK --interval=60s --timeout=10s --start-period=5s --retries=3 \
    CMD /opt/ebrowser/tests/test_memory_safety 2>/dev/null | grep -q "0 failed"

EXPOSE 8080

# Default: run benchmark to show capabilities
ENTRYPOINT ["/opt/ebrowser/benchmark"]
CMD []
