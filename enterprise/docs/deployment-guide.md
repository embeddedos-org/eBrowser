# eBrowser v2.0 — Deployment Guide

## Quick Start with Docker

```bash
# Build the image
cd eBrowser
docker build -t embeddedos/ebrowser:2.0.0 -f enterprise/docker/Dockerfile .

# Run the benchmark
docker run --rm embeddedos/ebrowser:2.0.0

# Run all tests
docker run --rm --entrypoint="" embeddedos/ebrowser:2.0.0 sh -c \
  "cd /opt/ebrowser/tests && ./test_memory_safety && ./test_sandbox && \
   ./test_firewall && ./test_anti_fingerprint && ./test_extension && \
   ./test_privacy && ./test_tab_manager && ./test_bookmark"

# Run load test (10,000 requests through full security pipeline)
docker run --rm --entrypoint="/opt/ebrowser/load_test" embeddedos/ebrowser:2.0.0

# Interactive shell
docker run --rm -it --entrypoint=/bin/bash embeddedos/ebrowser:2.0.0
```

## Docker Compose

```bash
cd eBrowser/enterprise/docker

# Run eBrowser
docker-compose up -d ebrowser

# Run benchmark
docker-compose --profile benchmark up benchmark

# Run tests
docker-compose --profile test up tests

# Run load test
docker-compose --profile loadtest up loadtest
```

## Kubernetes with Helm

```bash
cd eBrowser/enterprise/helm
helm install ebrowser . --namespace ebrowser --create-namespace
```

## Image Specifications

| Property | Value |
|----------|-------|
| Base Image | Ubuntu 22.04 (minimal) |
| Binary Size | ~200 KB |
| Image Size | ~80 MB (with SDL2 runtime) |
| Memory Limit | 128 MB |
| Security | Non-root, read-only rootfs, no-new-privileges |
| Capabilities | Only NET_BIND_SERVICE |

## Security Hardening

The Docker image follows security best practices:

1. **Multi-stage build** — build tools not included in runtime image
2. **Non-root user** — runs as `ebrowser` user (UID 1000)
3. **Read-only filesystem** — only `/tmp` and data volumes are writable
4. **No new privileges** — `no-new-privileges` security option
5. **Capability dropping** — all capabilities dropped except `NET_BIND_SERVICE`
6. **Resource limits** — 128MB memory, 2 CPU cores max
7. **Stripped binaries** — debug symbols removed
8. **Compiler hardening** — FORTIFY_SOURCE, stack protector, RELRO, PIE
9. **Health checks** — runs test_memory_safety every 60s

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `EBROWSER_SECURITY_LEVEL` | `strict` | Security level: `basic`, `standard`, `strict` |
| `EBROWSER_THEME` | `light` | UI theme: `light`, `dark`, `auto` |
| `EBROWSER_HTTPS_ONLY` | `true` | Force HTTPS for all connections |
| `EBROWSER_DOH_ENABLED` | `true` | DNS-over-HTTPS |
| `EBROWSER_TRACKER_BLOCKER` | `true` | Built-in tracker/ad blocker |
| `EBROWSER_ANTI_FINGERPRINT` | `standard` | Anti-fingerprinting level |

## Volumes

| Volume | Path | Purpose |
|--------|------|---------|
| `ebrowser_data` | `/opt/ebrowser/data` | User data (bookmarks, settings) |
| `ebrowser_extensions` | `/opt/ebrowser/data/extensions` | Installed extensions |
| `ebrowser_cache` | `/opt/ebrowser/data/cache` | Page cache |
