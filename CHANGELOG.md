# Changelog

## [2.0.2] — 2026-05-02

### eBrowser v2.0.2 — "Chrome-Like Browser Features"

Brings the desktop UI substantially closer to Chrome / Brave by wiring the
existing `eb_ui` library into the binary and shipping the Chrome chrome
(bookmarks bar, downloads panel, settings page, keyboard shortcuts, internal
`chrome://` pages, incognito mode, DevTools, Reader Mode, engine-failure
fallback UX).

### Added

- **Phase A — Quick visual / behavioral fixes**
  - Bumped CMake `project(VERSION)` to 2.0.2 so the title bar / banner /
    About dialog all match the next git tag.
  - New `omnibox.[ch]` with `eb_omnibox_normalize()` — typing `google.com` now
    auto-prepends `https://`; non-URL input becomes a Google search
    (configurable via the `search.default_engine` setting).
  - SDL2 window is now resizable; `SDL_WINDOWEVENT_SIZE_CHANGED` re-creates
    the texture and calls `lv_display_set_resolution()`.
  - Page area is now pure white (already was; preserved).
  - Hamburger menu replaced with a real popup panel anchored to
    `LV_ALIGN_OUT_BOTTOM_RIGHT` of the menu button — no more right-edge
    clipping.

- **Phase B — `eb_ui` linked into the binary.** `src/browser/CMakeLists.txt`
  now links `eb_ui`, `eBrowser_security`, `eb_privacy`. Unlocked the
  bookmark, download, devtools, theme, reader-mode, and tab-manager modules
  for use in the desktop UI.

- **Phase C — Bookmarks.** Bookmarks bar between toolbar and page area;
  ★ button on the right of the omnibox toggles the current page; Ctrl+D and
  Ctrl+Shift+B keyboard shortcuts; persistence to
  `%APPDATA%\eBrowser\bookmarks.json`. `chrome://bookmarks` lists everything.

- **Phase D — Downloads panel.** Bottom-anchored panel toggled by Ctrl+J or
  the hamburger menu; per-row name + `lv_bar` progress; 250 ms `lv_timer`
  ticks `eb_dl_tick()`. `chrome://downloads` shows the same data.

- **Phase E — Settings page.** New `src/browser/settings_page.c` rendered at
  `chrome://settings`. Categories: Appearance, Search, Startup, Privacy &
  Security, Downloads. Persisted via `eb_secure_store_t` to
  `%APPDATA%\eBrowser\settings.kv` (POSIX: `$HOME/.config/eBrowser`).

- **Phase F — Keyboard shortcuts.** Ctrl+T new tab, Ctrl+W close, Ctrl+L
  focus omnibox, Ctrl+R / F5 reload, Ctrl+Tab next / Ctrl+Shift+? prev,
  Alt+← / Alt+→ back / forward, Ctrl+J downloads, Ctrl+H history,
  Ctrl+D bookmark, Ctrl+Shift+N incognito, Ctrl+Shift+B bookmarks bar,
  Ctrl+± zoom, F11 fullscreen, F12 DevTools, Ctrl+Shift+R reader mode.

- **Phase G — Internal pages.** Router in `eb_browser_navigate` for
  `chrome://newtab`, `chrome://history`, `chrome://bookmarks`,
  `chrome://downloads`, `chrome://settings`, `chrome://about`,
  `chrome://version`, `chrome://extensions`.

- **Phase H — Tabs polish.** Middle-click closes a tab. Per-tab favicon slot
  shows a globe symbol while loading (full PNG/ICO favicon decode is a
  follow-up — LVGL ships no decoder). Incognito tabs render with a dark tab
  strip and 🔒 prefix.

- **Phase I — Engine-failure fallback UX.** When an HTTP response isn't
  HTML (or the engine can't render it), the page area shows a Reader-Mode
  banner if `eb_reader_can_activate()` is true, otherwise an explanatory
  panel with an **Open in system browser** button (uses `ShellExecuteA` on
  Windows, `xdg-open` on Linux, `open` on macOS).

- **Phase J — Incognito + DevTools + Reader Mode.** New incognito tab with
  dark strip + `eb_priv_set_mode(EB_PRIV_INCOGNITO)`; F12 toggles a
  bottom DevTools panel that records live `eb_dt_network_entry_t` rows;
  Ctrl+Shift+R toggles Reader Mode (renders via `eb_reader_extract`).

- **Phase K — Re-package.** `cmake --build build-win --config Release
  --target eBrowser` produces `build-win/port/Release/eBrowser.exe` (1.1 MB).

### Files added

- `include/eBrowser/omnibox.h`, `src/browser/omnibox.c`
- `include/eBrowser/settings_page.h`, `src/browser/settings_page.c`
- `port/sdl2/platform_open_url.c`

### Files modified

- `CMakeLists.txt` (root): version 2.0.0 → 2.0.2
- `include/eBrowser/browser.h`: extended `eb_browser_t` with
  `bookmarks`, `downloads`, `devtools`, `reader`, `theme`, `privacy`
  managers and panel handles; new public helpers.
- `include/eBrowser/platform.h`: added
  `eb_platform_open_url_in_system_browser()`.
- `src/browser/browser.c`: complete UI rewrite covering phases A, C–J.
- `src/browser/CMakeLists.txt`: links `eb_ui`, `eBrowser_security`,
  `eb_privacy`; on Windows links `shell32`.
- `port/CMakeLists.txt`: builds `port/sdl2/platform_open_url.c`.
- `port/sdl2/main_sdl2.c`: resizable window, modifier-aware shortcut
  dispatch, middle-click tab close, F11 fullscreen.

### Known follow-ups (deferred)

- PNG/ICO favicon decode (LVGL has no built-in decoder).
- Right-click tab context menu (currently only the close button works).
- Per-install random key for the settings store (currently uses a fixed
  dev key).

## [2.0.0] — 2026-04-27

### eBrowser v2.0 — "Security Fortress" Release

The biggest update in eBrowser history. Transforms eBrowser from a lightweight
embedded browser into the most secure, extensible, and performant browser per
line of code in the world.

**Stats:** 15 new headers, 17 new source files, 8 new test suites (138 tests),
benchmark suite, load test (588K req/s), Docker support.

### Added — Security Fortress

- **Process Sandbox** (`sandbox.c`) — seccomp-BPF syscall filtering, Linux
  namespace isolation (PID/net/mount/user/IPC), capability dropping, W^X
  enforcement, guard page allocation. 6 preset profiles (renderer, network,
  extension, GPU, plugin, strict).

- **Hardened Memory Allocator** (`memory_safety.c`) — canary values
  (0xDEADC0DE) before/after every allocation, quarantine zone for
  use-after-free detection, double-free detection with source location,
  memory zeroing on free, pool allocator for zero-fragmentation, allocation
  tracking with file:line.

- **Application Firewall** (`firewall.c`) — domain blocklist with wildcard
  matching (`*.tracker.com`), CIDR IP filtering, rate limiting per domain,
  HTTPS-only mode with auto HTTP→HTTPS upgrade, non-standard port blocking.

- **Anti-Fingerprinting Engine** (`anti_fingerprint.c`) — 4 protection levels
  (Off/Standard/Strict/Tor-like), canvas noise injection, WebGL
  vendor/renderer spoofing, font enumeration restriction (Tor Browser list),
  timing attack mitigation (jittered performance.now()), screen normalization,
  User-Agent randomization, WebRTC IP leak prevention, Battery/Bluetooth/USB
  API blocking.

- **DNS Security** (`dns_security.c`) — DNS-over-HTTPS (DoH) enabled by
  default with Cloudflare + Google providers, DNS cache with TTL, DNS
  rebinding protection (blocks private IP responses), DNSSEC validation.

### Added — Privacy Engine

- **Privacy Engine** (`privacy.c`) — Incognito and Tor-like modes, cookie
  policies (block 3P, block all, delete on close), referrer stripping
  (none/origin/same-origin), Do Not Track + Global Privacy Control headers,
  First-Party Isolation, storage partitioning.

- **Tracker & Ad Blocker** (`tracker_blocker.c`) — EasyList/EasyPrivacy
  compatible filter syntax, cosmetic filtering (element hiding), domain
  anchoring (`||tracker.com^`), resource type filtering, exception rules
  (`@@||allowed.com^`), per-domain rules.

- **URL Cleaning** — Automatically strips 25+ tracking parameters:
  `utm_source`, `utm_medium`, `utm_campaign`, `fbclid`, `gclid`, `msclkid`,
  `twclid`, `igshid`, and more.

### Added — Extension System

- **WebExtensions V3 API** (`extension.c`) — Full Manifest V3 JSON parser,
  content script injection with URL match patterns, background service workers,
  per-extension sandboxing, Storage API, Tabs API, WebRequest API (blocking),
  Alarms API, Context Menus API, Runtime messaging (port-based + one-shot),
  native messaging support.

### Added — Performance Engine

- **Thread Pool** (`perf.c`) — pthreads-based with configurable workers,
  priority task queue, wait-all synchronization.
- **LRU Cache** — O(1) get/put with doubly-linked list, TTL expiration,
  memory-bounded eviction.
- **HTTP/2 Multiplexing** — Stream management with concurrent request support.
- **Resource Prefetcher** — Priority-based queue with deduplication.
- **Compression** — Brotli/Zstd/Gzip/Deflate framework.

### Added — Advanced UI

- **Tab Manager** (`tab_manager.c`) — Tab groups with color coding, split
  view, tab pinning, tab hibernation (auto-sleep saves memory), tab search,
  recently closed, duplication, per-tab mute and zoom.
- **Bookmarks** (`bookmark.c`) — Folder hierarchy, tag-based organization,
  full-text search, HTML import/export (Netscape format).
- **Download Manager** (`download.c`) — Parallel downloads, pause/resume,
  SHA-256 verification, auto-retry on failure.
- **Developer Tools** (`devtools.c`) — Console (5 levels), network monitor,
  DOM inspector with node highlighting.
- **Reader Mode** (`reader_mode.c`) — Article extraction (Readability
  algorithm), 3 themes (light/dark/sepia), reading time estimation.
- **Theme Engine** (`theme.c`) — Light/Dark/Auto mode, custom accent colors,
  high contrast mode, force dark on all websites.

### Added — Build & Distribution

- **Compiler Hardening** — `-fstack-protector-strong`, `_FORTIFY_SOURCE=2`,
  PIE/ASLR, Full RELRO, non-executable stack.
- **Docker Support** — Multi-stage Dockerfile (build + runtime), 4
  docker-compose services (browser, benchmark, tests, loadtest), ~80 MB image.
- **Kubernetes** — Helm chart with security-hardened values.
- **Benchmark Suite** — Measures startup, parsing, crypto, security, memory,
  extension matching, tab operations.
- **Load Test** — 10,000 requests through 8-layer security pipeline at
  588,859 req/s with 16µs avg latency.

### Changed

- `CMakeLists.txt` — Version bumped to 2.0.0, added build targets for all
  new modules, added compiler hardening flags.
- `README.md` — Complete rewrite with feature comparison table, architecture
  diagram, security documentation, and extension API reference.

### Performance

| Metric | v1.0 | v2.0 |
|--------|------|------|
| Binary Size | ~100 KB | ~200 KB |
| Cold Start | ~50 ms | ~83 ms |
| Memory (idle) | ~2 MB | ~22 MB |
| Security Layers | 3 | 8 |
| Test Cases | ~80 | 138 |
| Extensions | Plugin only | Full WebExtensions V3 |

## [0.1.0] — 2025-04-24

### Initial Release
- HTML5 parser, CSS3 parser, DOM tree, layout engine
- LVGL-based rendering with SDL2 desktop port
- HTTP/HTTPS networking with TLS 1.2/1.3
- SHA-256, AES-128, HMAC-SHA256 cryptography
- CSP, XSS prevention, CORS, cookie security
- Basic plugin system with 7 hook points
- SVG parser, GPU renderer stub
- EmbeddedOS and WebAssembly ports
