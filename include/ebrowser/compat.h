// SPDX-License-Identifier: MIT
// Cross-platform compatibility shims for eBrowser
#ifndef eBrowser_COMPAT_H
#define eBrowser_COMPAT_H

// strcasestr requires _GNU_SOURCE on Linux (available natively on macOS/BSD)
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#ifdef _WIN32
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
// strcasestr not available on Windows — provide fallback
static inline const char *strcasestr(const char *haystack, const char *needle) {
    if (!needle[0]) return haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && ((*h >= 'A' && *h <= 'Z') ? *h + 32 : *h) ==
                           ((*n >= 'A' && *n <= 'Z') ? *n + 32 : *n)) { h++; n++; }
        if (!*n) return haystack;
    }
    return NULL;
}
#else
#include <strings.h>
#endif

// MAP_ANONYMOUS: Linux uses MAP_ANONYMOUS, some BSDs use MAP_ANON
#if defined(__APPLE__) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

// clock_gettime is available on macOS 10.12+ and all Linux
// No shim needed for modern systems

#endif // eBrowser_COMPAT_H
