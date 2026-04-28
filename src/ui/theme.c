// SPDX-License-Identifier: MIT
#include "eBrowser/theme.h"
#include <string.h>

void eb_theme_init(eb_theme_t *t) {
    if (!t) return; memset(t, 0, sizeof(*t));
    strcpy(t->font_ui, "Inter"); t->font_size_ui = 14; t->border_radius = 8;
    eb_theme_set_mode(t, EB_THEME_LIGHT);
}

void eb_theme_apply_light(eb_theme_t *t) {
    if (!t) return;
    t->colors.bg_primary    = 0xFFFFFF; t->colors.bg_secondary  = 0xF8F9FA;
    t->colors.bg_toolbar    = 0xF1F3F4; t->colors.bg_sidebar    = 0xF8F9FA;
    t->colors.bg_input      = 0xFFFFFF; t->colors.text_primary   = 0x202124;
    t->colors.text_secondary= 0x5F6368; t->colors.text_disabled  = 0x9AA0A6;
    t->colors.accent        = 0x1A73E8; t->colors.accent_hover   = 0x1967D2;
    t->colors.border        = 0xDADCE0; t->colors.error          = 0xD93025;
    t->colors.warning       = 0xF9AB00; t->colors.success        = 0x1E8E3E;
    t->colors.link          = 0x1A0DAB; t->colors.link_visited   = 0x681DA8;
    t->colors.tab_active    = 0xFFFFFF; t->colors.tab_inactive   = 0xE8EAED;
    t->colors.scrollbar     = 0xDADCE0;
}

void eb_theme_apply_dark(eb_theme_t *t) {
    if (!t) return;
    t->colors.bg_primary    = 0x202124; t->colors.bg_secondary  = 0x292A2D;
    t->colors.bg_toolbar    = 0x35363A; t->colors.bg_sidebar    = 0x292A2D;
    t->colors.bg_input      = 0x35363A; t->colors.text_primary   = 0xE8EAED;
    t->colors.text_secondary= 0x9AA0A6; t->colors.text_disabled  = 0x5F6368;
    t->colors.accent        = 0x8AB4F8; t->colors.accent_hover   = 0xAECBFA;
    t->colors.border        = 0x5F6368; t->colors.error          = 0xF28B82;
    t->colors.warning       = 0xFDD663; t->colors.success        = 0x81C995;
    t->colors.link          = 0x8AB4F8; t->colors.link_visited   = 0xC58AF9;
    t->colors.tab_active    = 0x35363A; t->colors.tab_inactive   = 0x292A2D;
    t->colors.scrollbar     = 0x5F6368;
}

void eb_theme_set_mode(eb_theme_t *t, eb_theme_mode_t mode) {
    if (!t) return; t->mode = mode;
    if (mode == EB_THEME_DARK) eb_theme_apply_dark(t);
    else eb_theme_apply_light(t);
}

void eb_theme_set_accent(eb_theme_t *t, uint32_t c) { if (t) t->colors.accent = c; }

const eb_color_scheme_t *eb_theme_get_colors(const eb_theme_t *t) {
    return t ? &t->colors : NULL;
}
