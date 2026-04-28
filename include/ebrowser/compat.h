// SPDX-License-Identifier: MIT
// Cross-platform compatibility shims for eBrowser
// Include this FIRST in every .c file for full portability.
#ifndef eBrowser_COMPAT_H
#define eBrowser_COMPAT_H

// _GNU_SOURCE for strcasestr on Linux (must be before any includes)
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#ifdef _WIN32
// --- Windows / MSVC ---

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <string.h>

// strcasecmp family
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

// strcasestr — not in MSVC, provide inline
static inline const char *strcasestr(const char *haystack, const char *needle) {
    if (!needle || !needle[0]) return haystack;
    for (; haystack && *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) { h++; n++; }
        if (!*n) return haystack;
    }
    return NULL;
}

// clock_gettime — not in MSVC, use QueryPerformanceCounter
#include <time.h>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME  0
#endif
struct timespec; // forward decl if needed
static inline int clock_gettime(int clk_id, struct timespec *ts) {
    (void)clk_id;
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    ts->tv_sec = (long)(count.QuadPart / freq.QuadPart);
    ts->tv_nsec = (long)((count.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart);
    return 0;
}

// mmap/munmap — not available on Windows, use VirtualAlloc
#include <memoryapi.h>
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_NONE   0x0
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void*)-1)

static inline void *mmap(void *addr, size_t len, int prot, int flags, int fd, long offset) {
    (void)addr; (void)flags; (void)fd; (void)offset;
    DWORD flProtect = PAGE_READWRITE;
    if (prot == PROT_NONE) flProtect = PAGE_NOACCESS;
    else if (prot == PROT_READ) flProtect = PAGE_READONLY;
    return VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, flProtect);
}

static inline int munmap(void *addr, size_t len) {
    (void)len;
    return VirtualFree(addr, 0, MEM_RELEASE) ? 0 : -1;
}

static inline int mprotect(void *addr, size_t len, int prot) {
    DWORD flProtect = PAGE_READWRITE, old;
    if (prot == PROT_NONE) flProtect = PAGE_NOACCESS;
    else if (prot == PROT_READ) flProtect = PAGE_READONLY;
    return VirtualProtect(addr, len, flProtect, &old) ? 0 : -1;
}

static inline long sysconf(int name) {
    (void)name;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (long)si.dwPageSize;
}
#define _SC_PAGESIZE 1

// MSG_NOSIGNAL doesn't exist on Windows
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#else
// --- POSIX (Linux, macOS, BSD) ---
#include <strings.h>

// MAP_ANONYMOUS: Linux uses MAP_ANONYMOUS, some BSDs use MAP_ANON
#if defined(__APPLE__) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

#endif // _WIN32

#endif // eBrowser_COMPAT_H
