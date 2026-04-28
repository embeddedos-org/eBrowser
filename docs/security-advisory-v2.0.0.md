# Security Advisory — eBrowser v2.0.0

**Date:** 2026-04-27
**Severity:** Informational (no vulnerabilities)
**Affects:** eBrowser v2.0.0

## Summary

eBrowser v2.0.0 ("Security Fortress") is a major release introducing 15 new
security and privacy modules. This advisory documents the security posture
of the release.

## Vulnerability Status

**No known vulnerabilities at time of release.**

### Testing Performed

| Test Type | Coverage | Result |
|-----------|----------|--------|
| Unit Tests | 138 test cases across 8 suites | All passing |
| Fuzz Testing | 5 targets (HTML, CSS, URL, XSS, Firewall) | No crashes found |
| AddressSanitizer | All fuzz targets | No issues |
| UndefinedBehaviorSanitizer | All fuzz targets | No issues |
| Valgrind (memcheck) | All 8 test suites | Clean |
| Static Analysis (cppcheck) | Full source tree | No critical issues |
| CodeQL | Automated on every push | Clean |

### Attack Surface Analysis

| Component | Input Source | Sanitization | Risk |
|-----------|-------------|-------------|------|
| HTML Parser | Untrusted web content | XSS scanner + sanitizer | Medium |
| CSS Parser | Untrusted stylesheets | Length/value bounds checking | Low |
| URL Parser | User input + links | Security check + privacy clean | Medium |
| JS Engine | Web scripts | Execution timeout + memory limit | High |
| Network | HTTP responses | TLS verification + CSP + HSTS | Medium |
| Extensions | Third-party code | Sandboxed + permission-gated | Medium |
| DNS | Network responses | DoH encryption + rebinding check | Low |

### Cryptographic Inventory

| Algorithm | Usage | Implementation | Status |
|-----------|-------|---------------|--------|
| SHA-256 | Content hashing, HMAC | Pure C (custom) | Verified against test vectors |
| AES-128-CBC | Secure storage | Pure C (custom) | Verified against NIST vectors |
| HMAC-SHA256 | Message authentication | Pure C (custom) | Verified |
| PBKDF2 | Key derivation | Pure C (SHA-256 based) | Verified |
| TLS 1.2/1.3 | Transport security | Structure only (needs backend) | Partial |

**Note:** The pure-C crypto implementations prioritize portability for embedded
targets. For production desktop use, linking OpenSSL/mbedTLS is recommended for
hardware-accelerated performance and FIPS compliance.

### Known Limitations

1. **TLS implementation** is structural — actual TLS handshake requires linking
   a TLS backend (OpenSSL, mbedTLS, or wolfSSL).
2. **JavaScript engine** is a basic interpreter — not a full V8/SpiderMonkey.
   Complex web applications will not work.
3. **Crypto performance** — Pure C SHA-256 is ~47µs/4KB vs ~2µs with AES-NI.
   Adequate for embedded, link OpenSSL for desktop.

### Compiler Hardening Matrix

| Protection | Linux (GCC) | Linux (Clang) | macOS | Windows (MSVC) |
|------------|------------|---------------|-------|----------------|
| Stack Protector | ✓ | ✓ | ✓ | /GS |
| FORTIFY_SOURCE | ✓ | ✓ | ✓ | N/A |
| PIE/ASLR | ✓ | ✓ | ✓ | /DYNAMICBASE |
| Full RELRO | ✓ | ✓ | N/A | N/A |
| NX Stack | ✓ | ✓ | ✓ | /NXCOMPAT |
| Stack Clash | ✓ | ✓ | N/A | N/A |
| Control Flow | N/A | -fsanitize=cfi | N/A | /guard:cf |

## Dependencies

See `sbom/ebrowser-2.0.0.cdx.json` for the full Software Bill of Materials.

| Dependency | Version | License | CVEs |
|------------|---------|---------|------|
| LVGL | 9.2 | MIT | None known |
| SDL2 | 2.x (optional) | Zlib | None critical |
| libc (system) | System | — | — |

**Total third-party dependencies: 2** (LVGL + optional SDL2)
**Zero npm/pip/cargo dependencies.**

## Recommendations

1. **Keep updated** — Subscribe to GitHub releases for security patches
2. **Use Clang** — Enables libFuzzer + additional sanitizers
3. **Enable all hardening** — Build with `cmake -DCMAKE_BUILD_TYPE=Release`
4. **Report vulnerabilities** — Email security@embeddedos.org
5. **Run fuzz tests** — `cmake -DeBrowser_BUILD_FUZZ=ON` before deploying

## Contact

- Security reports: security@embeddedos.org
- General issues: https://github.com/embeddedos-org/eBrowser/issues
