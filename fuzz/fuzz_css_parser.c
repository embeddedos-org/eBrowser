// SPDX-License-Identifier: MIT
// libFuzzer harness for eBrowser CSS parser
#include "eBrowser/css_parser.h"
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

    // Fuzz stylesheet parsing
    eb_stylesheet_t ss;
    eb_css_init_stylesheet(&ss);
    eb_css_parse(&ss, input, size);

    // Fuzz style resolution with parsed stylesheet
    eb_computed_style_t s1 = eb_css_resolve_style(&ss, "div", NULL, NULL);
    eb_computed_style_t s2 = eb_css_resolve_style(&ss, "p", "main", "content");
    (void)s1; (void)s2;

    // Fuzz inline style parsing
    eb_computed_style_t inline_s = eb_css_default_style("div");
    eb_css_apply_inline_style(&inline_s, input);

    // Fuzz color parsing with substrings of input
    if (size >= 4) {
        char color_buf[32];
        size_t clen = size < 31 ? size : 31;
        memcpy(color_buf, input, clen);
        color_buf[clen] = '\0';
        eb_css_parse_color(color_buf);
        eb_css_parse_length(color_buf);
    }

    // Fuzz specificity computation
    if (size < 128) {
        eb_css_compute_specificity(input);
    }

    free(input);
    return 0;
}
