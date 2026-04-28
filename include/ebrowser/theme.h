// SPDX-License-Identifier: MIT
#ifndef eBrowser_THEME_H
#define eBrowser_THEME_H
#include <stdbool.h>
#include <stdint.h>

typedef enum { EB_THEME_LIGHT, EB_THEME_DARK, EB_THEME_AUTO, EB_THEME_CUSTOM } eb_theme_mode_t;

typedef struct {
    uint32_t bg_primary, bg_secondary, bg_toolbar, bg_sidebar, bg_input;
    uint32_t text_primary, text_secondary, text_disabled;
    uint32_t accent, accent_hover, border, error, warning, success;
    uint32_t link, link_visited, tab_active, tab_inactive, scrollbar;
} eb_color_scheme_t;

typedef struct {
    eb_theme_mode_t mode; eb_color_scheme_t colors;
    char font_ui[128]; int font_size_ui, border_radius;
    bool high_contrast, reduce_motion, force_dark_pages;
    char custom_css[4096];
} eb_theme_t;

void  eb_theme_init(eb_theme_t *theme);
void  eb_theme_set_mode(eb_theme_t *theme, eb_theme_mode_t mode);
void  eb_theme_set_accent(eb_theme_t *theme, uint32_t color);
void  eb_theme_apply_light(eb_theme_t *theme);
void  eb_theme_apply_dark(eb_theme_t *theme);
const eb_color_scheme_t *eb_theme_get_colors(const eb_theme_t *theme);
#endif
