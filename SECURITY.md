# Security Policy

## Supported Versions

| Version | Supported          | End of Life |
|---------|--------------------|-------------|
| 2.0.x   | :white_check_mark: | Active      |
| 1.x     | :x:                | 2025-04-24  |
| < 1.0   | :x:                | Unsupported |

## Security Architecture

eBrowser v2.0 implements an 8-layer defense-in-depth security model:

```
Layer 8: Anti-Fingerprinting      — Canvas/WebGL/Font/Timing protection
Layer 7: Privacy Engine            — Tracker blocker, URL cleaning, GPC/DNT
Layer 6: Application Firewall      — Domain blocklist, CIDR, rate limiting
Layer 5: DNS Security              — DoH, DNSSEC, rebinding protection
Layer 4: Content Security          — CSP, XSS scanner, CORS, HSTS
Layer 3: Process Sandbox           — seccomp-BPF, namespaces, W^X
Layer 2: Memory Safety             — Canaries, quarantine, UAF detection
Layer 1: Compiler Hardening        — FORTIFY_SOURCE, PIE, RELRO, stack protector
```

## Security Features

### Compile-Time Hardening
- `-fstack-protector-strong` — Stack buffer overflow detection
- `-D_FORTIFY_SOURCE=2` — Runtime buffer overflow checks
- `-fPIE` + `-pie` — Address Space Layout Randomization (ASLR)
- `-Wl,-z,relro,-z,now` — Full RELRO (GOT hardening)
- `-Wl,-z,noexecstack` — Non-executable stack (NX)

### Runtime Protections
- **Process Sandbox** — seccomp-BPF syscall filtering, Linux namespace isolation
- **Memory Safety** — Canary values (0xDEADC0DE), quarantine zone for UAF detection, double-free detection, memory zeroing on free
- **W^X Enforcement** — No memory region is both writable and executable

### Continuous Security Testing
- **138 unit tests** — Including dedicated security test suites
- **5 libFuzzer harnesses** — HTML parser, CSS parser, URL parser, XSS scanner, firewall
- **AddressSanitizer** — Heap/stack overflow, use-after-free detection
- **UndefinedBehaviorSanitizer** — Integer overflow, null deref detection
- **Valgrind** — Nightly leak checks on all test suites
- **cppcheck** — Static analysis for common vulnerability patterns
- **CodeQL** — GitHub-integrated semantic code analysis

## Reporting a Vulnerability

**Email:** security@embeddedos.org

> Do **NOT** open public GitHub issues for security vulnerabilities.

### What to Include

1. Affected component and version
2. Step-by-step reproduction instructions
3. Proof-of-concept (code, crash dump, fuzzer input)
4. Impact assessment (confidentiality, integrity, availability)
5. Suggested CVSS score if applicable

### Response SLA

| Phase            | Timeline  |
|------------------|-----------|
| Acknowledge      | 24 hours  |
| Triage + assign  | 72 hours  |
| Fix developed    | 30 days   |
| Patch released   | 90 days   |
| Public disclosure | 90 days (coordinated) |

## Security Advisories

### v2.0.0 (2026-04-27) — No known vulnerabilities

This is a new release with comprehensive security review:
- All code has been fuzz-tested with ASan/UBSan
- Memory safety module detects buffer overflows at runtime
- No external dependencies with known CVEs (pure C, only LVGL for rendering)

## Safe Harbor

We consider security research conducted in good faith to be authorized and will not pursue legal action against researchers who follow responsible disclosure practices.

## Bug Bounty

We currently do not operate a formal bug bounty program. Critical vulnerability reporters will be credited in release notes and the SECURITY.md hall of fame.

## SBOM

A Software Bill of Materials (SBOM) in CycloneDX format is available at:
`sbom/ebrowser-2.0.0.cdx.json`
