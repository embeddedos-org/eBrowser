// SPDX-License-Identifier: MIT
//
// Settings page (chrome://settings) — Phase E.
//
// Provides a Chrome/Brave-inspired settings panel rendered into a parent
// LVGL container. Settings are persisted via eb_secure_store_t to
// %APPDATA%\eBrowser\settings.kv on Windows or $HOME/.config/eBrowser on
// POSIX systems. The store password is a fixed dev-key for v2.0.2 (see
// EB_SETTINGS_DEV_KEY). Replacing this with a per-install random key is a
// follow-up.
//
// Public surface (declared in settings_page.h):
//   - eb_settings_init / eb_settings_deinit
//   - eb_settings_render(parent_container)
//   - eb_settings_get_string / eb_settings_set_string
//   - eb_settings_get_bool   / eb_settings_set_bool
//
#include "eBrowser/settings_page.h"
#include "eBrowser/secure_storage.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#  include <shlobj.h>
#endif

#define EB_SETTINGS_DEV_KEY "ebrowser-dev-key-v2"

/* Module-private singleton — settings UI is a global panel like Chrome's. */
static eb_secure_store_t s_store;
static bool s_store_open = false;

static void resolve_settings_path(char *out, size_t outsz) {
#if defined(_WIN32)
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        snprintf(out, outsz, "%s\\eBrowser", appdata);
        CreateDirectoryA(out, NULL);
        snprintf(out, outsz, "%s\\eBrowser\\settings.kv", appdata);
        return;
    }
#endif
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(out, outsz, "%s/.config/eBrowser/settings.kv", home);
}

bool eb_settings_init(void) {
    if (s_store_open) return true;
    char path[512];
    resolve_settings_path(path, sizeof(path));

    if (eb_store_init(&s_store, path) != EB_STORE_OK) return false;
    if (eb_store_open(&s_store, EB_SETTINGS_DEV_KEY) != EB_STORE_OK) return false;
    /* Best-effort load — empty/missing file is fine for first launch. */
    (void)eb_store_load(&s_store);
    s_store_open = true;
    return true;
}

void eb_settings_deinit(void) {
    if (!s_store_open) return;
    eb_store_save(&s_store);
    eb_store_close(&s_store);
    s_store_open = false;
}

bool eb_settings_get_string(const char *key, char *value, size_t max) {
    if (!s_store_open || !key || !value || max == 0) return false;
    return eb_store_get_string(&s_store, key, value, max) == EB_STORE_OK;
}

bool eb_settings_set_string(const char *key, const char *value) {
    if (!s_store_open || !key || !value) return false;
    if (eb_store_put_string(&s_store, key, value) != EB_STORE_OK) return false;
    eb_store_save(&s_store);
    return true;
}

bool eb_settings_get_bool(const char *key, bool dflt) {
    char buf[8];
    if (!eb_settings_get_string(key, buf, sizeof(buf))) return dflt;
    return buf[0] == '1' || buf[0] == 't' || buf[0] == 'T';
}

bool eb_settings_set_bool(const char *key, bool val) {
    return eb_settings_set_string(key, val ? "1" : "0");
}

/* ===== UI ============================================================== */

typedef struct {
    const char *key;
    const char *label;
    const char *deflt;
} eb_settings_text_t;

typedef struct {
    const char *key;
    const char *label;
    bool deflt;
} eb_settings_bool_t;

static const eb_settings_text_t kTextSettings[] = {
    {"appearance.theme",       "Theme (light/dark/auto)",            "auto"},
    {"appearance.font_size",   "UI font size (px)",                  "14"},
    {"search.default_engine",  "Default search engine URL prefix",
        "https://www.google.com/search?q="},
    {"startup.behavior",       "On startup (newtab/restore/url)",    "newtab"},
    {"startup.url",            "Startup URL (if behavior=url)",      "https://www.google.com"},
    {"downloads.directory",    "Downloads folder",                    ""},
};

static const eb_settings_bool_t kBoolSettings[] = {
    {"appearance.show_bookmarks_bar", "Show bookmarks bar",            true},
    {"privacy.dnt",                   "Send Do Not Track requests",    true},
    {"privacy.gpc",                   "Send Global Privacy Control",   true},
    {"privacy.https_only",            "HTTPS-only mode",               false},
    {"privacy.incognito_on_launch",   "Always start in incognito",     false},
    {"downloads.ask_where",           "Ask where to save each file",   false},
};

static void text_changed_cb(lv_event_t *e) {
    const eb_settings_text_t *s = (const eb_settings_text_t *)lv_event_get_user_data(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (!s || !ta) return;
    eb_settings_set_string(s->key, lv_textarea_get_text(ta));
}

static void bool_changed_cb(lv_event_t *e) {
    const eb_settings_bool_t *s = (const eb_settings_bool_t *)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    if (!s || !sw) return;
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    eb_settings_set_bool(s->key, on);
}

static lv_obj_t *make_section(lv_obj_t *parent, const char *title) {
    lv_obj_t *hdr = lv_label_create(parent);
    lv_label_set_text(hdr, title);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(hdr, lv_color_make(0x1A, 0x73, 0xE8), LV_PART_MAIN);
    lv_obj_set_style_pad_top(hdr, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(hdr, 4, LV_PART_MAIN);
    return hdr;
}

void eb_settings_render(lv_obj_t *parent) {
    if (!parent) return;
    if (!s_store_open) eb_settings_init();

    lv_obj_clean(parent);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(parent, lv_color_white(), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);

    /* ===== Appearance / Search / Startup / Privacy / Downloads sections ===== */
    static const char *kSectionTitles[] = {
        "Appearance", "Search", "Startup", "Privacy & Security", "Downloads",
    };
    /* Map setting prefixes to section indices. */
    const struct { const char *prefix; int section; } kRouting[] = {
        {"appearance.", 0}, {"search.", 1}, {"startup.", 2},
        {"privacy.", 3}, {"downloads.", 4},
    };
    const int kNumSections = (int)(sizeof(kSectionTitles) / sizeof(kSectionTitles[0]));
    const int kNumRoute    = (int)(sizeof(kRouting) / sizeof(kRouting[0]));

    for (int sec = 0; sec < kNumSections; sec++) {
        make_section(parent, kSectionTitles[sec]);

        for (size_t i = 0; i < sizeof(kTextSettings) / sizeof(kTextSettings[0]); i++) {
            int my_section = -1;
            for (int r = 0; r < kNumRoute; r++) {
                if (strncmp(kTextSettings[i].key, kRouting[r].prefix,
                            strlen(kRouting[r].prefix)) == 0) {
                    my_section = kRouting[r].section;
                    break;
                }
            }
            if (my_section != sec) continue;

            lv_obj_t *row = lv_obj_create(parent);
            lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_style_pad_all(row, 4, LV_PART_MAIN);
            lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);

            lv_obj_t *lbl = lv_label_create(row);
            lv_label_set_text(lbl, kTextSettings[i].label);
            lv_obj_set_width(lbl, 280);

            lv_obj_t *ta = lv_textarea_create(row);
            lv_textarea_set_one_line(ta, true);
            lv_obj_set_flex_grow(ta, 1);
            lv_obj_set_height(ta, 32);

            char cur[256];
            if (!eb_settings_get_string(kTextSettings[i].key, cur, sizeof(cur))) {
                strncpy(cur, kTextSettings[i].deflt, sizeof(cur) - 1);
                cur[sizeof(cur) - 1] = '\0';
            }
            lv_textarea_set_text(ta, cur);
            lv_obj_add_event_cb(ta, text_changed_cb, LV_EVENT_DEFOCUSED,
                                (void *)&kTextSettings[i]);
        }

        for (size_t i = 0; i < sizeof(kBoolSettings) / sizeof(kBoolSettings[0]); i++) {
            int my_section = -1;
            for (int r = 0; r < kNumRoute; r++) {
                if (strncmp(kBoolSettings[i].key, kRouting[r].prefix,
                            strlen(kRouting[r].prefix)) == 0) {
                    my_section = kRouting[r].section;
                    break;
                }
            }
            if (my_section != sec) continue;

            lv_obj_t *row = lv_obj_create(parent);
            lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_style_pad_all(row, 4, LV_PART_MAIN);
            lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);

            lv_obj_t *lbl = lv_label_create(row);
            lv_label_set_text(lbl, kBoolSettings[i].label);
            lv_obj_set_width(lbl, 280);

            lv_obj_t *sw = lv_switch_create(row);
            if (eb_settings_get_bool(kBoolSettings[i].key, kBoolSettings[i].deflt)) {
                lv_obj_add_state(sw, LV_STATE_CHECKED);
            }
            lv_obj_add_event_cb(sw, bool_changed_cb, LV_EVENT_VALUE_CHANGED,
                                (void *)&kBoolSettings[i]);
        }
    }
}
