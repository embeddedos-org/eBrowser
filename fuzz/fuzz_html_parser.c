// SPDX-License-Identifier: MIT
// libFuzzer harness for eBrowser HTML parser
// Catches: buffer overflows, infinite loops, memory corruption, crashes
#include "eBrowser/html_parser.h"
#include "eBrowser/dom.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 65536) return 0;

    // Ensure null-terminated input
    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    // Fuzz the full HTML parse pipeline
    eb_dom_node_t *doc = eb_html_parse(input, size);
    if (doc) {
        // Exercise DOM traversal on parsed result
        eb_dom_node_t *body = eb_dom_find_by_tag(doc, "body");
        eb_dom_node_t *head = eb_dom_find_by_tag(doc, "head");
        (void)body; (void)head;

        // Search operations
        eb_dom_node_list_t list = {0};
        eb_dom_find_all_by_tag(doc, "div", &list);
        eb_dom_find_all_by_tag(doc, "a", &list);
        eb_dom_find_all_by_class(doc, "test", &list);
        eb_dom_find_by_id(doc, "main");

        // Get text content
        char buf[4096];
        eb_dom_get_inner_text(doc, buf, sizeof(buf));

        // Clone and destroy
        eb_dom_node_t *clone = eb_dom_clone_node(doc, true);
        if (clone) eb_dom_destroy(clone);

        eb_dom_destroy(doc);
    }

    // Also fuzz tokenizer independently
    eb_tokenizer_t tok;
    eb_tokenizer_init(&tok, input, size);
    for (int i = 0; i < 1000; i++) { // limit iterations
        eb_html_token_t t = eb_tokenizer_next(&tok);
        if (t.type == EB_TOK_EOF) break;
    }

    free(input);
    return 0;
}
