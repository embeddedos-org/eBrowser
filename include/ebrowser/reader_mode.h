// SPDX-License-Identifier: MIT
#ifndef eBrowser_READER_MODE_H
#define eBrowser_READER_MODE_H
#include <stdbool.h>
#include <stddef.h>
#include "eBrowser/dom.h"

typedef enum { EB_READER_LIGHT, EB_READER_DARK, EB_READER_SEPIA } eb_reader_theme_t;

typedef struct {
    bool active; eb_reader_theme_t theme; int font_size, line_height, max_width;
    char font_family[128], title[512], byline[256];
    char *content; size_t content_len;
    int word_count, reading_time_min, scroll_percent;
} eb_reader_mode_t;

void  eb_reader_init(eb_reader_mode_t *rm);
void  eb_reader_destroy(eb_reader_mode_t *rm);
bool  eb_reader_can_activate(const eb_dom_node_t *doc);
bool  eb_reader_extract(eb_reader_mode_t *rm, const eb_dom_node_t *doc);
void  eb_reader_set_theme(eb_reader_mode_t *rm, eb_reader_theme_t theme);
void  eb_reader_set_font_size(eb_reader_mode_t *rm, int size);
#endif
