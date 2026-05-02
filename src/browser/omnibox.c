// SPDX-License-Identifier: MIT
// Omnibox normalization — see include/eBrowser/omnibox.h.
#include "eBrowser/omnibox.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char *kRecognisedSchemes[] = {
    "http://",
    "https://",
    "file://",
    "chrome://",
    "about:",
    "ftp://",
    NULL,
};

bool eb_omnibox_has_scheme(const char *s) {
    if (!s) return false;
    for (const char **p = kRecognisedSchemes; *p; p++) {
        size_t n = strlen(*p);
        /* case-insensitive scheme compare */
        size_t i = 0;
        for (; i < n && s[i]; i++) {
            char a = s[i];
            char b = (*p)[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) break;
        }
        if (i == n) return true;
    }
    return false;
}

/* RFC-ish hostname detector. Accepts:
 *   foo.com[/path][?query][#frag]
 *   foo.bar.baz
 *   127.0.0.1, [::1]
 *   localhost
 *   anything containing ":<digits>/" (host:port style)
 * Rejects strings with internal whitespace.
 */
bool eb_omnibox_looks_like_url(const char *s) {
    if (!s || !*s) return false;
    if (strchr(s, ' ') || strchr(s, '\t')) return false;

    /* localhost (with optional port/path) */
    if (strncmp(s, "localhost", 9) == 0 &&
        (s[9] == '\0' || s[9] == '/' || s[9] == ':' || s[9] == '?' || s[9] == '#')) {
        return true;
    }

    /* IPv6-literal in brackets */
    if (s[0] == '[') return true;

    /* Walk up to the first '/' '?' '#' or end — that span is the host. */
    const char *end = s;
    while (*end && *end != '/' && *end != '?' && *end != '#') end++;
    size_t host_len = (size_t)(end - s);
    if (host_len == 0 || host_len > 253) return false;

    bool has_dot = false;
    bool has_alnum = false;
    for (size_t i = 0; i < host_len; i++) {
        char c = s[i];
        if (c == '.') {
            has_dot = true;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9')) {
            has_alnum = true;
        } else if (c == '-' || c == ':') {
            /* allowed (port separator, hyphen) */
        } else {
            return false;
        }
    }
    return has_dot && has_alnum;
}

/* Conservative URL-encoder for query strings — encodes anything that isn't
 * unreserved or one of '-' '_' '.' '~'. */
static size_t url_encode(const char *src, char *dst, size_t dstsz) {
    static const char hex[] = "0123456789ABCDEF";
    size_t o = 0;
    for (; *src; src++) {
        unsigned char c = (unsigned char)*src;
        bool unreserved = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') ||
                          c == '-' || c == '_' || c == '.' || c == '~';
        if (unreserved) {
            if (o + 1 >= dstsz) break;
            dst[o++] = (char)c;
        } else if (c == ' ') {
            if (o + 1 >= dstsz) break;
            dst[o++] = '+';
        } else {
            if (o + 3 >= dstsz) break;
            dst[o++] = '%';
            dst[o++] = hex[(c >> 4) & 0xF];
            dst[o++] = hex[c & 0xF];
        }
    }
    if (o < dstsz) dst[o] = '\0';
    else if (dstsz) dst[dstsz - 1] = '\0';
    return o;
}

bool eb_omnibox_normalize(const char *user_input, char *out, size_t outsz,
                          const char *search_prefix) {
    if (!user_input || !out || outsz < 2) return false;

    /* Trim leading/trailing whitespace. */
    while (*user_input == ' ' || *user_input == '\t') user_input++;
    if (*user_input == '\0') return false;

    char trimmed[2048];
    size_t in_len = strlen(user_input);
    if (in_len >= sizeof(trimmed)) in_len = sizeof(trimmed) - 1;
    memcpy(trimmed, user_input, in_len);
    trimmed[in_len] = '\0';
    while (in_len > 0 && (trimmed[in_len - 1] == ' ' || trimmed[in_len - 1] == '\t' ||
                          trimmed[in_len - 1] == '\n' || trimmed[in_len - 1] == '\r')) {
        trimmed[--in_len] = '\0';
    }
    if (in_len == 0) return false;

    if (eb_omnibox_has_scheme(trimmed)) {
        size_t n = (in_len < outsz - 1) ? in_len : outsz - 1;
        memcpy(out, trimmed, n);
        out[n] = '\0';
        return true;
    }

    if (eb_omnibox_looks_like_url(trimmed)) {
        int n = snprintf(out, outsz, "https://%s", trimmed);
        return n > 0 && (size_t)n < outsz;
    }

    /* Treat as search query. */
    const char *prefix = search_prefix ? search_prefix : EB_OMNIBOX_DEFAULT_SEARCH;
    size_t plen = strlen(prefix);
    if (plen + 1 >= outsz) return false;
    memcpy(out, prefix, plen);
    url_encode(trimmed, out + plen, outsz - plen);
    return true;
}
