// SPDX-License-Identifier: MIT
#include "eBrowser/platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(eBrowser_PLATFORM_EOS)
/* EoS platform stubs */
#else
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

eb_platform_type_t eb_platform_detect(void) {
#if defined(_WIN32)
    return EB_PLATFORM_WINDOWS;
#elif defined(eBrowser_PLATFORM_EOS)
    return EB_PLATFORM_RTOS;
#elif defined(eBrowser_PLATFORM_WEB)
    return EB_PLATFORM_WEB;
#elif defined(__linux__)
    return EB_PLATFORM_LINUX;
#else
    return EB_PLATFORM_UNKNOWN;
#endif
}

const char *eb_platform_name(void) {
    switch (eb_platform_detect()) {
        case EB_PLATFORM_LINUX:     return "Linux";
        case EB_PLATFORM_RTOS:      return "RTOS";
        case EB_PLATFORM_BAREMETAL: return "Bare-metal";
        case EB_PLATFORM_WINDOWS:   return "Windows";
        case EB_PLATFORM_WEB:       return "Web";
        default:                     return "Unknown";
    }
}

uint32_t eb_platform_tick_ms(void) {
#if defined(_WIN32)
    return (uint32_t)GetTickCount();
#elif defined(eBrowser_PLATFORM_EOS) || defined(eBrowser_PLATFORM_WEB)
    return 0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

void eb_platform_sleep_ms(uint32_t ms) {
#if defined(_WIN32)
    Sleep(ms);
#elif defined(eBrowser_PLATFORM_EOS) || defined(eBrowser_PLATFORM_WEB)
    (void)ms;
#else
    usleep(ms * 1000);
#endif
}

void *eb_platform_alloc(size_t size) {
    return malloc(size);
}

void eb_platform_free(void *ptr) {
    free(ptr);
}

size_t eb_platform_available_memory(void) {
#if defined(_WIN32)
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        return (size_t)ms.ullAvailPhys;
    }
    return 0;
#elif defined(__linux__)
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[128];
    size_t avail = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long val;
        if (sscanf(line, "MemAvailable: %lu kB", &val) == 1) {
            avail = (size_t)val * 1024;
            break;
        }
    }
    fclose(f);
    return avail;
#elif defined(eBrowser_PLATFORM_EOS) || defined(eBrowser_PLATFORM_WEB)
    return 0;
#else
    return 0;
#endif
}

bool eb_platform_file_exists(const char *path) {
    if (!path) return false;
#if defined(_WIN32)
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES);
#elif defined(eBrowser_PLATFORM_EOS) || defined(eBrowser_PLATFORM_WEB)
    (void)path;
    return false;
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

int eb_platform_read_file(const char *path, char **out_buf, size_t *out_len) {
    if (!path || !out_buf || !out_len) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return -1; }
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return -1; }
    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[read] = 0;
    *out_buf = buf;
    *out_len = read;
    return 0;
}
