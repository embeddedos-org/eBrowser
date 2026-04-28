// SPDX-License-Identifier: MIT
#include "eBrowser/reader_mode.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void eb_reader_init(eb_reader_mode_t *rm) {
    if (!rm) return; memset(rm, 0, sizeof(*rm));
    rm->theme = EB_READER_LIGHT; rm->font_size = 18;
    rm->line_height = 160; rm->max_width = 700;
    strcpy(rm->font_family, "Georgia");
}

void eb_reader_destroy(eb_reader_mode_t *rm) {
    if (!rm) return; free(rm->content); memset(rm, 0, sizeof(*rm));
}

bool eb_reader_can_activate(const eb_dom_node_t *doc) {
    if (!doc) return false;
    /* Check for <article>, <p> tags - heuristic for article content */
    eb_dom_node_t *article = eb_dom_find_by_tag((eb_dom_node_t*)doc, "article");
    if (article) return true;
    /* Count <p> tags - if many, likely readable content */
    eb_dom_node_list_t list = {0};
    eb_dom_find_all_by_tag((eb_dom_node_t*)doc, "p", &list);
    return list.count >= 3;
}

static int count_words(const char *text) {
    if (!text) return 0;
    int n = 0; bool in_word = false;
    for (; *text; text++) {
        if (isspace((unsigned char)*text)) in_word = false;
        else if (!in_word) { in_word = true; n++; }
    }
    return n;
}

bool eb_reader_extract(eb_reader_mode_t *rm, const eb_dom_node_t *doc) {
    if (!rm || !doc) return false;
    /* Simplified Readability: find <article> or largest <div> with most <p> children */
    eb_dom_node_t *article = eb_dom_find_by_tag((eb_dom_node_t*)doc, "article");
    eb_dom_node_t *target = article;
    if (!target) {
        /* Find div with most text content */
        eb_dom_node_list_t divs = {0};
        eb_dom_find_all_by_tag((eb_dom_node_t*)doc, "div", &divs);
        int best_p = 0;
        for (int i = 0; i < divs.count; i++) {
            eb_dom_node_list_t ps = {0};
            eb_dom_find_all_by_tag(divs.nodes[i], "p", &ps);
            if (ps.count > best_p) { best_p = ps.count; target = divs.nodes[i]; }
        }
    }
    if (!target) return false;

    /* Extract title */
    eb_dom_node_t *h1 = eb_dom_find_by_tag((eb_dom_node_t*)doc, "h1");
    if (h1) eb_dom_get_inner_text(h1, rm->title, sizeof(rm->title));

    /* Extract text content - simplified */
    char buf[16384] = {0};
    eb_dom_get_inner_text(target, buf, sizeof(buf));
    rm->content = strdup(buf);
    rm->content_len = strlen(buf);
    rm->word_count = count_words(buf);
    rm->reading_time_min = (rm->word_count + 199) / 200; /* ~200 WPM */
    rm->active = true;
    return true;
}

void eb_reader_set_theme(eb_reader_mode_t *rm, eb_reader_theme_t t) { if (rm) rm->theme = t; }
void eb_reader_set_font_size(eb_reader_mode_t *rm, int sz) { if (rm && sz >= 10 && sz <= 36) rm->font_size = sz; }
