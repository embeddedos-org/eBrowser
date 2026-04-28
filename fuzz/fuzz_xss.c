// SPDX-License-Identifier: MIT
// libFuzzer harness for eBrowser XSS scanner and sanitizer
#include "eBrowser/security.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 65536) return 0;

    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    // Fuzz XSS scanning
    eb_xss_scan_html(input, size);

    // Fuzz HTML sanitization
    char *sanitized = (char *)malloc(size * 6 + 1); // worst case entity expansion
    if (sanitized) {
        eb_xss_sanitize_html(input, size, sanitized, size * 6);
        free(sanitized);
    }

    // Fuzz HTML escaping
    char *escaped = (char *)malloc(size * 6 + 1);
    if (escaped) {
        eb_xss_escape_html(input, escaped, size * 6);
        free(escaped);
    }

    // Fuzz attribute safety checks with substrings
    if (size >= 2) {
        size_t mid = size / 2;
        char attr[256] = {0}, val[256] = {0};
        size_t alen = mid < 255 ? mid : 255;
        size_t vlen = (size - mid) < 255 ? (size - mid) : 255;
        memcpy(attr, input, alen);
        memcpy(val, input + mid, vlen);
        eb_xss_is_safe_attribute(attr, val);
    }

    // Fuzz CSP parsing
    eb_csp_policy_t csp;
    eb_csp_init(&csp);
    eb_csp_parse(&csp, input);
    eb_csp_allows_source(&csp, EB_CSP_SCRIPT_SRC, "https://example.com");
    eb_csp_allows_source(&csp, EB_CSP_DEFAULT_SRC, input);

    free(input);
    return 0;
}
