// SPDX-License-Identifier: MIT
// libFuzzer harness for eBrowser URL parser
#include "eBrowser/url.h"
#include "eBrowser/security.h"
#include "eBrowser/privacy.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 8192) return 0;

    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    // Fuzz URL parsing
    eb_url_t url;
    eb_url_parse(input, &url);

    // Fuzz URL security checks
    eb_security_check_url(input);
    eb_xss_is_safe_url(input);
    eb_security_is_secure_origin(input);
    eb_security_is_mixed_content("https://example.com", input);
    eb_security_is_mixed_content(input, "https://example.com");

    // Fuzz URL cleaning
    static eb_privacy_t priv;
    static int init = 0;
    if (!init) { eb_priv_init(&priv); init = 1; }
    char clean[8192];
    eb_priv_clean_url(&priv, input, clean, sizeof(clean));

    free(input);
    return 0;
}
