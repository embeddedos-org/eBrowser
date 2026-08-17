# eBrowser — Embedded Web Browser

[![CI](https://github.com/embeddedos-org/eBrowser/actions/workflows/ci.yml/badge.svg)](https://github.com/embeddedos-org/eBrowser/actions/workflows/ci.yml)
[![CodeQL](https://github.com/embeddedos-org/eBrowser/actions/workflows/codeql.yml/badge.svg)](https://github.com/embeddedos-org/eBrowser/actions/workflows/codeql.yml)
[![Scorecard](https://github.com/embeddedos-org/eBrowser/actions/workflows/scorecard.yml/badge.svg)](https://github.com/embeddedos-org/eBrowser/actions/workflows/scorecard.yml)
[![Release](https://github.com/embeddedos-org/eBrowser/actions/workflows/release.yml/badge.svg)](https://github.com/embeddedos-org/eBrowser/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A lightweight, privacy-oriented web browser written in **C (C11)**, built for
embedded targets and desktops. The UI is rendered with [LVGL](https://lvgl.io)
and TLS is provided by [mbedTLS](https://github.com/Mbed-TLS/mbedtls). eBrowser
is part of the **EmbeddedOS (EoS)** ecosystem and can be built for the EoS
platform, SDL2 (desktop), or WebAssembly (Emscripten).

> Scope note: eBrowser implements its own small HTML/CSS/DOM/layout engine aimed
> at constrained devices. It is not a Chromium/WebKit/Servo replacement and does
> not aim for full web-platform parity.

## Features (by build module)

Build modules are toggled through CMake options in `CMakeLists.txt`. All are on
by default except the JavaScript engine and fuzzing.

| Module | Source | Purpose |
|---|---|---|
| Engine | `src/engine/` | HTML parser, CSS parser, DOM, layout, SVG parser, GPU renderer, optional JS engine |
| Render | `src/render/` | Rendering backend |
| Network | `src/network/` | HTTP, cookies, cache, URL handling |
| Input | `src/input/` | Input abstraction layer |
| Security | `src/security/` | Crypto, TLS, permissions, secure storage |
| Sandbox / hardening | `src/security/{sandbox,memory,firewall,anti_fingerprint,dns}/` | Process sandbox, memory-safety helpers, app firewall, anti-fingerprinting, DNS security |
| Privacy | `src/privacy/` | Tracker blocker and privacy controls |
| Extensions | `src/extensions/` | WebExtensions support |
| Perf | `src/perf/` | Performance engine |
| UI | `src/ui/` | DevTools, downloads, bookmarks, reader mode, themes, tab manager |
| Telemetry / Plugin | `src/telemetry/`, `src/plugin/` | Logging/metrics and plugin system |
| Browser app | `src/browser/` | Browser shell, omnibox, settings page |

Optional feature flags (default value): `eBrowser_BUILD_JS` (OFF),
`eBrowser_BUILD_FUZZ` (OFF), `eBrowser_USE_MBEDTLS` (ON). See `CMakeLists.txt`
for the full list.

## Repository layout

```
src/                Core browser modules (C)
include/ebrowser/   Public headers
platform/           Platform abstraction (platform.c)
port/               Platform ports: eos/, sdl2/, web/ (Emscripten)
extern/             Vendored config + fetched deps (LVGL, mbedTLS)
extension/          Browser extension assets
web-app/            Vite web front-end companion
mobile/             Expo mobile companion
enterprise/         Enterprise packaging
installer/          Installer assets
fuzz/               Fuzzing harnesses (Clang, opt-in)
tests/              C tests + Python test suites
docs/               Architecture, platform guide, site, book
assets/             Static assets
```

## Build

The project builds with CMake (>= 3.16). LVGL and mbedTLS are fetched
automatically via `FetchContent` if not vendored under `extern/`.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Platform selection is derived from the toolchain: an `eos.cmake` toolchain file
selects the EoS platform, an Emscripten build selects the Web platform,
otherwise the SDL2 desktop platform is used.

A zero-prerequisite helper script is provided for Linux/macOS (installs missing
tooling where it can, then configures and builds):

```bash
./setup.sh            # Linux/macOS
setup.bat             # Windows
./setup_macos.sh      # macOS-specific
```

## Test

C tests are registered with CTest when configured with testing enabled:

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build
```

There is also a Python test suite (unit / functional / performance / simulation)
run via:

```bash
python run_all_tests.py    # runs: pytest tests/unit tests/functional tests/performance tests/simulation -v
```

## Documentation

- `docs/architecture.md` — architecture overview
- `docs/platform-guide.md` — porting/platform notes
- `docs/book/` — long-form documentation
- `SECURITY.md`, `CONTRIBUTING.md`, `CHANGELOG.md`

## License

MIT — see [LICENSE](LICENSE).
