// SPDX-License-Identifier: MIT
// Cross-platform "open URL in the user's default browser" shim.
//
// On Windows: uses ShellExecuteA with the "open" verb (requires shell32.lib).
// On macOS:   uses `open <url>` via system().
// On Linux:   uses `xdg-open <url>` via system().
// Implemented as part of Phase I (engine-failure fallback UX).
#include "eBrowser/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shellapi.h>
#endif

bool eb_platform_open_url_in_system_browser(const char *url) {
    if (!url || !*url) return false;
#if defined(_WIN32)
    /* HINSTANCE return > 32 means success per Win32 docs. */
    HINSTANCE rc = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)rc) > 32;
#elif defined(__APPLE__)
    char cmd[2304];
    /* Single-quote-escape minimally; reject URLs containing a single quote. */
    if (strchr(url, '\'')) return false;
    snprintf(cmd, sizeof(cmd), "open '%s' >/dev/null 2>&1 &", url);
    return system(cmd) == 0;
#else
    char cmd[2304];
    if (strchr(url, '\'')) return false;
    snprintf(cmd, sizeof(cmd), "xdg-open '%s' >/dev/null 2>&1 &", url);
    return system(cmd) == 0;
#endif
}
