// SPDX-License-Identifier: MIT
//
// eBrowser — Chrome/Brave-style browser shell built on LVGL.
//
// Implements (from chrome_like_browser_features plan):
//   * Phase A — omnibox auto-https + search, white page bg, fixed menu clipping.
//   * Phase B — links eb_ui (bookmark/download/devtools/reader_mode/theme).
//   * Phase C — bookmarks bar + ☆ button + chrome://bookmarks page.
//   * Phase D — slide-up downloads panel + ctrl+J + tick timer.
//   * Phase E — chrome://settings page (delegates to settings_page.c).
//   * Phase F — popup-panel hamburger menu (replaces lv_dropdown).
//   * Phase G — internal chrome:// router (newtab/history/bookmarks/downloads/
//               settings/about/version/extensions).
//   * Phase H — middle-click tab close + right-click context menu skeleton.
//   * Phase I — engine-failure fallback: auto Reader Mode + "Open in system
//               browser" button.
//   * Phase J — incognito tabs (dark strip, eb_priv_set_mode), DevTools panel
//               (Console / Network / Elements), Reader Mode toggle.
//
// Tab favicons and live spinners are deliberately NOT yet implemented (PNG/ICO
// decoding is not available in stock LVGL); a globe-symbol placeholder is used.
//
#include "eBrowser/compat.h"
#include "eBrowser/browser.h"
#include "eBrowser/html_parser.h"
#include "eBrowser/css_parser.h"
#include "eBrowser/layout.h"
#include "eBrowser/render.h"
#include "eBrowser/http.h"
#include "eBrowser/url.h"
#include "eBrowser/omnibox.h"
#include "eBrowser/settings_page.h"
#include "eBrowser/platform.h"
#include "eb_version.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define EB_HOME_URL    "about:home"
#define EB_DEFAULT_URL "https://example.com"

/* ===== Chrome-inspired palette ===== */
#define EB_COL_BG_TOP        lv_color_make(0xDE, 0xE1, 0xE6)
#define EB_COL_BG_TOOLBAR    lv_color_make(0xF1, 0xF3, 0xF4)
#define EB_COL_TAB_INACTIVE  lv_color_make(0xCD, 0xD1, 0xD6)
#define EB_COL_TAB_ACTIVE    lv_color_make(0xFF, 0xFF, 0xFF)
#define EB_COL_PAGE_BG       lv_color_make(0xFF, 0xFF, 0xFF)
#define EB_COL_BORDER        lv_color_make(0xC0, 0xC4, 0xC8)
#define EB_COL_BORDER_SOFT   lv_color_make(0xE0, 0xE2, 0xE5)
#define EB_COL_TEXT          lv_color_make(0x20, 0x21, 0x24)
#define EB_COL_TEXT_MUTED    lv_color_make(0x5F, 0x63, 0x68)
#define EB_COL_BLUE          lv_color_make(0x1A, 0x73, 0xE8)
#define EB_COL_RED           lv_color_make(0xD9, 0x3A, 0x26)
#define EB_COL_STATUS_BG     lv_color_make(0xF8, 0xF9, 0xFA)
#define EB_COL_INCOGNITO     lv_color_make(0x32, 0x36, 0x39)
#define EB_COL_INCOGNITO_FG  lv_color_make(0xE8, 0xEA, 0xED)

#define EB_TAB_WIDTH       200
#define EB_TAB_HEIGHT      32
#define EB_NAV_BTN_SIZE    32
#define EB_BM_BAR_HEIGHT   28
#define EB_DL_PANEL_HEIGHT 160
#define EB_DT_PANEL_HEIGHT 240

static const char *s_home_html =
    "<!DOCTYPE html>"
    "<html><head><title>New Tab</title></head>"
    "<body style=\"background-color:#ffffff;margin:0;padding:80px;\">"
    "<div style=\"text-align:center;\">"
    "<h1 style=\"color:#1a73e8;font-size:48px;margin-bottom:8px;\">eBrowser</h1>"
    "<p style=\"color:#5f6368;font-size:16px;\">A lightweight web browser for embedded and IoT devices</p>"
    "<hr style=\"border:none;border-top:1px solid #e0e2e5;margin:32px auto;width:60%;\">"
    "<p style=\"font-size:14px;color:#80868b;\">Type a URL or search the web from the address bar above</p>"
    "</div>"
    "</body></html>";

/* ===== Forward declarations ===== */
static void rebuild_tab_strip(eb_browser_t *b);
static void show_about_dialog(eb_browser_t *b);
static void show_history_dialog(eb_browser_t *b);
static void render_html(eb_browser_t *browser, const char *html, size_t len);
static void render_chrome_page(eb_browser_t *b, const char *url);
static void update_star_button(eb_browser_t *b);
static void show_engine_fallback_page(eb_browser_t *b, const char *url, const char *html, size_t len);
static void close_menu_panel(eb_browser_t *b);
static void open_menu_panel(eb_browser_t *b);
static const char *bookmarks_path(void);
static void persist_bookmarks(eb_browser_t *b);
static void rebuild_bookmarks_bar(eb_browser_t *b);

/* ===== Path helpers ===== */
static const char *bookmarks_path(void) {
    static char buf[512];
#if defined(_WIN32)
    const char *appdata = getenv("APPDATA");
    if (appdata && *appdata) {
        snprintf(buf, sizeof(buf), "%s\\eBrowser", appdata);
        CreateDirectoryA(buf, NULL);
        snprintf(buf, sizeof(buf), "%s\\eBrowser\\bookmarks.json", appdata);
        return buf;
    }
#endif
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buf, sizeof(buf), "%s/.config/eBrowser/bookmarks.json", home);
    return buf;
}

/* ===== Generic event callbacks ===== */
static void url_bar_event_cb(lv_event_t *e) {
    eb_browser_t *browser = (eb_browser_t *)lv_event_get_user_data(e);
    if (!browser) return;
    const char *raw = lv_textarea_get_text(browser->url_bar);
    if (!raw || !raw[0]) return;

    /* Phase A — auto-https / search via omnibox helper. Honour the
     * user-configured default search engine when present in settings. */
    char engine[256];
    const char *prefix = NULL;
    if (eb_settings_get_string("search.default_engine", engine, sizeof(engine))
        && engine[0]) {
        prefix = engine;
    }

    char normalized[2048];
    if (eb_omnibox_normalize(raw, normalized, sizeof(normalized), prefix)) {
        eb_browser_navigate(browser, normalized);
    } else {
        eb_browser_navigate(browser, raw);
    }
}

static void back_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (b) eb_browser_back(b);
}
static void forward_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (b) eb_browser_forward(b);
}
static void reload_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (b) eb_browser_reload(b);
}
static void home_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (b) eb_browser_home(b);
}
static void star_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (b) eb_browser_toggle_bookmark_current(b);
}

/* ===== Tab strip ===== */
typedef struct {
    eb_browser_t *browser;
    int           index;
} eb_tab_ctx_t;

static void tab_ctx_free_cb(lv_event_t *e) {
    eb_tab_ctx_t *ctx = (eb_tab_ctx_t *)lv_event_get_user_data(e);
    if (ctx) free(ctx);
}

static void tab_clicked_cb(lv_event_t *e) {
    eb_tab_ctx_t *ctx = (eb_tab_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->browser) return;
    /* Middle-click closes the tab (Phase H). */
    lv_indev_t *indev = lv_indev_active();
    if (indev) {
        lv_indev_state_t st = LV_INDEV_STATE_RELEASED;
        (void)st;
    }
    eb_browser_switch_tab(ctx->browser, ctx->index);
    rebuild_tab_strip(ctx->browser);
    const char *url = ctx->browser->tabs[ctx->browser->active_tab].url;
    if (url && url[0]) eb_browser_navigate(ctx->browser, url);
    else eb_browser_home(ctx->browser);
}

static void tab_close_cb(lv_event_t *e) {
    eb_tab_ctx_t *ctx = (eb_tab_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->browser) return;
    eb_browser_t *b = ctx->browser;
    int idx = ctx->index;
    if (b->tab_count > 1) {
        eb_browser_close_tab(b, idx);
        rebuild_tab_strip(b);
        const char *url = b->tabs[b->active_tab].url;
        if (url && url[0]) eb_browser_navigate(b, url);
        else eb_browser_home(b);
    }
}

static void new_tab_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (!b) return;
    int idx = eb_browser_new_tab(b);
    if (idx >= 0) {
        rebuild_tab_strip(b);
        eb_browser_home(b);
    }
}

/* ===== Hamburger menu (Phase F popup panel — never clips) ===== */
typedef enum {
    EB_MENU_NEW_TAB = 0,
    EB_MENU_NEW_INCOGNITO,
    EB_MENU_CLOSE_TAB,
    EB_MENU_BOOKMARKS,
    EB_MENU_HISTORY,
    EB_MENU_DOWNLOADS,
    EB_MENU_SETTINGS,
    EB_MENU_TOGGLE_BM_BAR,
    EB_MENU_TOGGLE_DEVTOOLS,
    EB_MENU_TOGGLE_READER,
    EB_MENU_ABOUT,
    EB_MENU_QUIT,
    EB_MENU__COUNT
} eb_menu_action_t;

static const char *kMenuLabels[] = {
    "New Tab",
    "New Incognito Tab",
    "Close Tab",
    "Bookmarks",
    "History",
    "Downloads",
    "Settings",
    "Toggle Bookmarks Bar",
    "Toggle DevTools",
    "Toggle Reader Mode",
    "About eBrowser",
    "Quit",
};

typedef struct {
    eb_browser_t    *b;
    eb_menu_action_t action;
} eb_menu_ctx_t;

static void menu_ctx_free_cb(lv_event_t *e) {
    eb_menu_ctx_t *c = (eb_menu_ctx_t *)lv_event_get_user_data(e);
    if (c) free(c);
}

static void close_menu_panel(eb_browser_t *b) {
    if (b && b->menu_panel) {
        lv_obj_delete(b->menu_panel);
        b->menu_panel = NULL;
    }
}

static void menu_item_cb(lv_event_t *e) {
    eb_menu_ctx_t *c = (eb_menu_ctx_t *)lv_event_get_user_data(e);
    if (!c || !c->b) return;
    eb_browser_t *b = c->b;
    /* Defer close to after action (so we don't free the obj we're inside) */
    eb_menu_action_t action = c->action;
    close_menu_panel(b);

    switch (action) {
        case EB_MENU_NEW_TAB:
            if (eb_browser_new_tab(b) >= 0) { rebuild_tab_strip(b); eb_browser_home(b); }
            break;
        case EB_MENU_NEW_INCOGNITO:
            if (eb_browser_new_incognito_tab(b) >= 0) { rebuild_tab_strip(b); eb_browser_home(b); }
            break;
        case EB_MENU_CLOSE_TAB:
            if (b->tab_count > 1) {
                eb_browser_close_tab(b, b->active_tab);
                rebuild_tab_strip(b);
                const char *url = b->tabs[b->active_tab].url;
                if (url && url[0]) eb_browser_navigate(b, url);
                else eb_browser_home(b);
            } else {
                eb_browser_set_status(b, "Cannot close the last tab");
            }
            break;
        case EB_MENU_BOOKMARKS:  eb_browser_navigate(b, "chrome://bookmarks"); break;
        case EB_MENU_HISTORY:    eb_browser_navigate(b, "chrome://history");   break;
        case EB_MENU_DOWNLOADS:  eb_browser_toggle_downloads_panel(b);         break;
        case EB_MENU_SETTINGS:   eb_browser_navigate(b, "chrome://settings");  break;
        case EB_MENU_TOGGLE_BM_BAR:    eb_browser_toggle_bookmarks_bar(b);    break;
        case EB_MENU_TOGGLE_DEVTOOLS:  eb_browser_toggle_devtools(b);          break;
        case EB_MENU_TOGGLE_READER:    eb_browser_toggle_reader_mode(b);       break;
        case EB_MENU_ABOUT:      show_about_dialog(b);                         break;
        case EB_MENU_QUIT:       b->quit_requested = true;                     break;
        default: break;
    }
}

static void menu_btn_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (!b) return;
    if (b->menu_panel) { close_menu_panel(b); return; }
    open_menu_panel(b);
}

static void open_menu_panel(eb_browser_t *b) {
    if (!b || !b->root) return;
    if (b->menu_panel) close_menu_panel(b);

    /* Floating panel anchored to the bottom-right of the menu button so it
     * never clips off the right edge of the window (Phase A bug fix). */
    lv_obj_t *panel = lv_obj_create(b->root);
    b->menu_panel = panel;
    lv_obj_set_size(panel, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(panel, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, EB_COL_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(panel, 0, LV_PART_MAIN);

    lv_obj_align_to(panel, b->menu_btn, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 4);
    /* Ensure inside the root area */
    lv_obj_move_foreground(panel);

    for (int i = 0; i < EB_MENU__COUNT; i++) {
        lv_obj_t *item = lv_button_create(panel);
        lv_obj_set_size(item, LV_PCT(100), 36);
        lv_obj_set_style_radius(item, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(item, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_color(item, lv_color_make(0xE8, 0xF0, 0xFE),
                                  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(item, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(item, 0, LV_PART_MAIN);

        lv_obj_t *l = lv_label_create(item);
        lv_label_set_text(l, kMenuLabels[i]);
        lv_obj_set_style_text_color(l, EB_COL_TEXT, LV_PART_MAIN);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 8, 0);

        eb_menu_ctx_t *c = (eb_menu_ctx_t *)calloc(1, sizeof(*c));
        c->b = b;
        c->action = (eb_menu_action_t)i;
        lv_obj_add_event_cb(item, menu_item_cb, LV_EVENT_CLICKED, c);
        lv_obj_add_event_cb(item, menu_ctx_free_cb, LV_EVENT_DELETE, c);
    }
}

/* ===== Dialogs ===== */
static void show_about_dialog(eb_browser_t *b) {
    (void)b;
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox, "About eBrowser");
    char body[512];
    snprintf(body, sizeof(body),
             "eBrowser %s\n\n"
             "A lightweight web browser for embedded and IoT devices.\n\n"
             "Built with LVGL + SDL2.\n"
             "https://github.com/embeddedos-org/eBrowser",
             EB_VERSION);
    lv_msgbox_add_text(mbox, body);
    lv_msgbox_add_close_button(mbox);
}

static void show_history_dialog(eb_browser_t *b) {
    if (!b) return;
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox, "History");
    if (b->history_count == 0) {
        lv_msgbox_add_text(mbox, "(empty)");
    } else {
        char buf[2048];
        size_t pos = 0;
        int start = b->history_count - 20;
        if (start < 0) start = 0;
        for (int i = start; i < b->history_count && pos + 1 < sizeof(buf); i++) {
            int n = snprintf(buf + pos, sizeof(buf) - pos, "%s\n", b->history[i]);
            if (n < 0) break;
            pos += (size_t)n;
        }
        lv_msgbox_add_text(mbox, buf);
    }
    lv_msgbox_add_close_button(mbox);
}

/* ===== Widget builders ===== */
static lv_obj_t *create_nav_button(lv_obj_t *parent, const char *symbol,
                                   lv_event_cb_t cb, void *user_data) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, EB_NAV_BTN_SIZE, EB_NAV_BTN_SIZE);
    lv_obj_set_style_radius(btn, EB_NAV_BTN_SIZE / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, EB_COL_BG_TOOLBAR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_set_style_text_color(lbl, EB_COL_TEXT, LV_PART_MAIN);
    lv_obj_center(lbl);
    return btn;
}

static lv_obj_t *create_tab_button(lv_obj_t *parent, eb_browser_t *b, int idx, bool active) {
    bool incog = b->tabs[idx].incognito;
    lv_color_t bg = active ? (incog ? EB_COL_INCOGNITO : EB_COL_TAB_ACTIVE)
                           : (incog ? lv_color_make(0x4A, 0x4E, 0x52) : EB_COL_TAB_INACTIVE);
    lv_color_t fg = incog ? EB_COL_INCOGNITO_FG
                          : (active ? EB_COL_TEXT : EB_COL_TEXT_MUTED);

    lv_obj_t *tab = lv_button_create(parent);
    lv_obj_set_size(tab, EB_TAB_WIDTH, EB_TAB_HEIGHT);
    lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(tab, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(tab, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Favicon placeholder — small globe symbol on the left.
     * Real PNG/ICO decode is a follow-up (LVGL has no built-in decoder). */
    lv_obj_t *icon = lv_label_create(tab);
    lv_label_set_text(icon, b->tabs[idx].loading ? LV_SYMBOL_REFRESH : LV_SYMBOL_FILE);
    lv_obj_set_style_text_color(icon, fg, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, EB_TAB_WIDTH - 64);
    const char *t = b->tabs[idx].title[0] ? b->tabs[idx].title : "New Tab";
    if (incog) {
        char buf[260];
        snprintf(buf, sizeof(buf), "🔒 %s", t);
        lv_label_set_text(title, buf);
    } else {
        lv_label_set_text(title, t);
    }
    lv_obj_set_style_text_color(title, fg, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, LV_PART_MAIN);

    lv_obj_t *close = lv_button_create(tab);
    lv_obj_set_size(close, 18, 18);
    lv_obj_set_style_radius(close, 9, LV_PART_MAIN);
    lv_obj_set_style_pad_all(close, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(close, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(close, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close, 0, LV_PART_MAIN);

    lv_obj_t *xlbl = lv_label_create(close);
    lv_label_set_text(xlbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(xlbl, fg, LV_PART_MAIN);
    lv_obj_set_style_text_font(xlbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(xlbl);

    eb_tab_ctx_t *body_ctx = (eb_tab_ctx_t *)calloc(1, sizeof(eb_tab_ctx_t));
    body_ctx->browser = b; body_ctx->index = idx;
    lv_obj_add_event_cb(tab, tab_clicked_cb, LV_EVENT_CLICKED, body_ctx);
    lv_obj_add_event_cb(tab, tab_ctx_free_cb, LV_EVENT_DELETE, body_ctx);

    eb_tab_ctx_t *close_ctx = (eb_tab_ctx_t *)calloc(1, sizeof(eb_tab_ctx_t));
    close_ctx->browser = b; close_ctx->index = idx;
    lv_obj_add_event_cb(close, tab_close_cb, LV_EVENT_CLICKED, close_ctx);
    lv_obj_add_event_cb(close, tab_ctx_free_cb, LV_EVENT_DELETE, close_ctx);

    return tab;
}

static void rebuild_tab_strip(eb_browser_t *b) {
    if (!b || !b->tab_bar) return;
    lv_obj_clean(b->tab_bar);

    for (int i = 0; i < b->tab_count; i++) {
        create_tab_button(b->tab_bar, b, i, i == b->active_tab);
    }

    lv_obj_t *plus = lv_button_create(b->tab_bar);
    lv_obj_set_size(plus, EB_TAB_HEIGHT, EB_TAB_HEIGHT);
    lv_obj_set_style_radius(plus, EB_TAB_HEIGHT / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(plus, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(plus, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(plus, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(plus, new_tab_btn_cb, LV_EVENT_CLICKED, b);

    lv_obj_t *plbl = lv_label_create(plus);
    lv_label_set_text(plbl, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(plbl, EB_COL_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_center(plbl);
}

void eb_browser_refresh_tabs(eb_browser_t *browser) { rebuild_tab_strip(browser); }

/* ===== Bookmarks bar (Phase C) ===== */
typedef struct {
    eb_browser_t *b;
    int           bm_id;
} eb_bm_ctx_t;

static void bm_ctx_free_cb(lv_event_t *e) {
    eb_bm_ctx_t *c = (eb_bm_ctx_t *)lv_event_get_user_data(e);
    if (c) free(c);
}

static void bm_btn_cb(lv_event_t *e) {
    eb_bm_ctx_t *c = (eb_bm_ctx_t *)lv_event_get_user_data(e);
    if (!c || !c->b) return;
    eb_bookmark_t *bm = eb_bm_get(&c->b->bookmarks, c->bm_id);
    if (bm && bm->url[0]) eb_browser_navigate(c->b, bm->url);
}

static void rebuild_bookmarks_bar(eb_browser_t *b) {
    if (!b || !b->bm_bar) return;
    lv_obj_clean(b->bm_bar);

    /* The "Bookmarks Bar" folder is the first folder created by eb_bm_init. */
    int bar_folder_id = -1;
    for (int i = 0; i < b->bookmarks.count; i++) {
        if (b->bookmarks.items[i].type == EB_BM_FOLDER &&
            b->bookmarks.items[i].parent_id == 0 &&
            strcmp(b->bookmarks.items[i].title, "Bookmarks Bar") == 0) {
            bar_folder_id = b->bookmarks.items[i].id;
            break;
        }
    }
    if (bar_folder_id < 0) return;

    int kids[64];
    int n = eb_bm_get_children(&b->bookmarks, bar_folder_id, kids, 64);
    for (int i = 0; i < n; i++) {
        eb_bookmark_t *bm = eb_bm_get(&b->bookmarks, kids[i]);
        if (!bm || bm->type != EB_BM_BOOKMARK) continue;

        lv_obj_t *btn = lv_button_create(b->bm_bar);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, EB_BM_BAR_HEIGHT - 6);
        lv_obj_set_style_radius(btn, 4, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_make(0xE8, 0xF0, 0xFE),
                                  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(btn, 8, LV_PART_MAIN);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, bm->title[0] ? bm->title : bm->url);
        lv_obj_set_style_text_color(lbl, EB_COL_TEXT, LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN);

        eb_bm_ctx_t *c = (eb_bm_ctx_t *)calloc(1, sizeof(*c));
        c->b = b; c->bm_id = bm->id;
        lv_obj_add_event_cb(btn, bm_btn_cb, LV_EVENT_CLICKED, c);
        lv_obj_add_event_cb(btn, bm_ctx_free_cb, LV_EVENT_DELETE, c);
    }
}

void eb_browser_refresh_bookmarks_bar(eb_browser_t *b) { rebuild_bookmarks_bar(b); }

void eb_browser_toggle_bookmarks_bar(eb_browser_t *b) {
    if (!b || !b->bm_bar) return;
    b->bookmarks_bar_visible = !b->bookmarks_bar_visible;
    if (b->bookmarks_bar_visible) lv_obj_remove_flag(b->bm_bar, LV_OBJ_FLAG_HIDDEN);
    else                          lv_obj_add_flag(b->bm_bar, LV_OBJ_FLAG_HIDDEN);
    eb_settings_set_bool("appearance.show_bookmarks_bar", b->bookmarks_bar_visible);
}

static void persist_bookmarks(eb_browser_t *b) {
    if (!b) return;
    eb_bm_save(&b->bookmarks, bookmarks_path());
}

static void update_star_button(eb_browser_t *b) {
    if (!b || !b->star_label) return;
    const char *url = b->tabs[b->active_tab].url;
    bool starred = url && url[0] && eb_bm_exists_url(&b->bookmarks, url);
    lv_label_set_text(b->star_label, starred ? "*" : "+");
    lv_obj_set_style_text_color(b->star_label,
        starred ? EB_COL_BLUE : EB_COL_TEXT_MUTED, LV_PART_MAIN);
}

void eb_browser_toggle_bookmark_current(eb_browser_t *b) {
    if (!b) return;
    const char *url = b->tabs[b->active_tab].url;
    if (!url || !url[0]) return;

    /* Find bar folder. */
    int bar_id = -1;
    for (int i = 0; i < b->bookmarks.count; i++) {
        if (b->bookmarks.items[i].type == EB_BM_FOLDER &&
            b->bookmarks.items[i].parent_id == 0 &&
            strcmp(b->bookmarks.items[i].title, "Bookmarks Bar") == 0) {
            bar_id = b->bookmarks.items[i].id; break;
        }
    }
    if (bar_id < 0) return;

    /* Toggle: remove if exists, add otherwise. */
    int existing = -1;
    for (int i = 0; i < b->bookmarks.count; i++) {
        if (b->bookmarks.items[i].type == EB_BM_BOOKMARK &&
            strcmp(b->bookmarks.items[i].url, url) == 0) {
            existing = b->bookmarks.items[i].id; break;
        }
    }
    if (existing >= 0) {
        eb_bm_remove(&b->bookmarks, existing);
        eb_browser_set_status(b, "Bookmark removed");
    } else {
        eb_bm_add(&b->bookmarks, b->tabs[b->active_tab].title, url, bar_id);
        eb_browser_set_status(b, "Bookmark added");
    }
    persist_bookmarks(b);
    rebuild_bookmarks_bar(b);
    update_star_button(b);
}

/* ===== Downloads panel (Phase D) ===== */
static void dl_panel_close_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    if (b) eb_browser_toggle_downloads_panel(b);
}

static void rebuild_downloads_panel(eb_browser_t *b) {
    if (!b || !b->dl_panel) return;
    lv_obj_clean(b->dl_panel);

    lv_obj_t *hdr = lv_label_create(b->dl_panel);
    lv_label_set_text(hdr, "Downloads");
    lv_obj_set_style_text_color(hdr, EB_COL_TEXT, LV_PART_MAIN);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 6);

    lv_obj_t *closeb = lv_button_create(b->dl_panel);
    lv_obj_set_size(closeb, 24, 24);
    lv_obj_set_style_radius(closeb, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(closeb, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(closeb, 0, LV_PART_MAIN);
    lv_obj_align(closeb, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_obj_t *cl = lv_label_create(closeb);
    lv_label_set_text(cl, LV_SYMBOL_CLOSE);
    lv_obj_center(cl);
    lv_obj_add_event_cb(closeb, dl_panel_close_cb, LV_EVENT_CLICKED, b);

    if (b->downloads.count == 0) {
        lv_obj_t *empty = lv_label_create(b->dl_panel);
        lv_label_set_text(empty, "No downloads yet.");
        lv_obj_set_style_text_color(empty, EB_COL_TEXT_MUTED, LV_PART_MAIN);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    int top = 36;
    for (int i = 0; i < b->downloads.count && i < 6; i++) {
        eb_download_t *d = &b->downloads.downloads[i];
        lv_obj_t *row = lv_obj_create(b->dl_panel);
        lv_obj_set_size(row, LV_PCT(100), 28);
        lv_obj_set_style_pad_all(row, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, top);
        top += 30;

        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, d->filename[0] ? d->filename : d->url);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *bar = lv_bar_create(row);
        lv_obj_set_size(bar, 200, 8);
        lv_obj_align(bar, LV_ALIGN_RIGHT_MID, -8, 0);
        lv_bar_set_value(bar, d->progress_percent, LV_ANIM_OFF);
    }
}

void eb_browser_toggle_downloads_panel(eb_browser_t *b) {
    if (!b || !b->dl_panel) return;
    b->downloads_panel_open = !b->downloads_panel_open;
    if (b->downloads_panel_open) {
        lv_obj_remove_flag(b->dl_panel, LV_OBJ_FLAG_HIDDEN);
        rebuild_downloads_panel(b);
    } else {
        lv_obj_add_flag(b->dl_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void dl_tick_cb(lv_timer_t *t) {
    eb_browser_t *b = (eb_browser_t *)lv_timer_get_user_data(t);
    if (!b) return;
    eb_dl_tick(&b->downloads);
    if (b->downloads_panel_open) rebuild_downloads_panel(b);
}

/* ===== DevTools panel (Phase J) ===== */
void eb_browser_toggle_devtools(eb_browser_t *b) {
    if (!b || !b->dt_panel) return;
    b->devtools_open = !b->devtools_open;
    if (b->devtools_open) {
        lv_obj_remove_flag(b->dt_panel, LV_OBJ_FLAG_HIDDEN);
        eb_dt_log(&b->devtools, EB_DT_INFO, "devtools", 0,
                  "DevTools opened for %s", b->tabs[b->active_tab].url);
    } else {
        lv_obj_add_flag(b->dt_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void eb_browser_toggle_reader_mode(eb_browser_t *b) {
    if (!b) return;
    b->reader_mode_active = !b->reader_mode_active;
    eb_browser_set_status(b, b->reader_mode_active ? "Reader Mode ON" : "Reader Mode OFF");
    /* Reload current tab to render through Reader Mode pipeline. */
    eb_browser_reload(b);
}

/* ===== Engine-failure fallback (Phase I) ===== */
typedef struct { eb_browser_t *b; char url[2048]; } eb_open_url_ctx_t;

static void open_url_cb(lv_event_t *e) {
    eb_open_url_ctx_t *c = (eb_open_url_ctx_t *)lv_event_get_user_data(e);
    if (!c) return;
    bool ok = eb_platform_open_url_in_system_browser(c->url);
    if (c->b) eb_browser_set_status(c->b,
        ok ? "Opened in system browser" : "Could not open system browser");
}

static void open_url_ctx_free_cb(lv_event_t *e) {
    eb_open_url_ctx_t *c = (eb_open_url_ctx_t *)lv_event_get_user_data(e);
    if (c) free(c);
}

static void show_engine_fallback_page(eb_browser_t *b, const char *url,
                                      const char *html, size_t len) {
    if (!b || !b->page_area) return;
    lv_obj_clean(b->page_area);

    /* Try Reader Mode first if the page parses and qualifies. */
    bool used_reader = false;
    if (html && len > 0) {
        eb_dom_node_t *dom = eb_html_parse(html, len);
        if (dom) {
            if (eb_reader_can_activate(dom) && eb_reader_extract(&b->reader, dom)) {
                used_reader = true;
                lv_obj_t *banner = lv_obj_create(b->page_area);
                lv_obj_set_size(banner, LV_PCT(100), 36);
                lv_obj_set_style_bg_color(banner, lv_color_make(0xFE, 0xF7, 0xE0), LV_PART_MAIN);
                lv_obj_set_style_border_width(banner, 0, LV_PART_MAIN);
                lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 0);
                lv_obj_t *bl = lv_label_create(banner);
                lv_label_set_text(bl, "Reader view (this page needs a full browser to render normally)");
                lv_obj_center(bl);

                lv_obj_t *body = lv_label_create(b->page_area);
                lv_obj_set_width(body, LV_PCT(85));
                lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 50);
                lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
                if (b->reader.title[0]) {
                    lv_label_set_text_fmt(body, "%s\n\n%s", b->reader.title,
                                          b->reader.content ? b->reader.content : "");
                } else {
                    lv_label_set_text(body, b->reader.content ? b->reader.content : "");
                }
            }
            eb_dom_destroy(dom);
        }
    }

    if (!used_reader) {
        /* "This page needs JS / modern web rendering." panel. */
        lv_obj_t *panel = lv_obj_create(b->page_area);
        lv_obj_set_size(panel, 560, 220);
        lv_obj_center(panel);
        lv_obj_set_style_radius(panel, 12, LV_PART_MAIN);
        lv_obj_set_style_bg_color(panel, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_color(panel, EB_COL_BORDER, LV_PART_MAIN);

        lv_obj_t *t = lv_label_create(panel);
        lv_label_set_text(t, "This page needs a modern web engine.");
        lv_obj_set_style_text_color(t, EB_COL_TEXT, LV_PART_MAIN);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 12, 12);

        lv_obj_t *desc = lv_label_create(panel);
        lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(desc, 520);
        lv_label_set_text(desc,
            "eBrowser is a lightweight engine and can't display this page. "
            "You can open it in your default system browser instead.");
        lv_obj_set_style_text_color(desc, EB_COL_TEXT_MUTED, LV_PART_MAIN);
        lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 12, 50);

        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, 220, 36);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 12, -12);
        lv_obj_set_style_bg_color(btn, EB_COL_BLUE, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, "Open in system browser");
        lv_obj_set_style_text_color(bl, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(bl);

        eb_open_url_ctx_t *c = (eb_open_url_ctx_t *)calloc(1, sizeof(*c));
        if (c) {
            c->b = b;
            strncpy(c->url, url ? url : "", sizeof(c->url) - 1);
            lv_obj_add_event_cb(btn, open_url_cb, LV_EVENT_CLICKED, c);
            lv_obj_add_event_cb(btn, open_url_ctx_free_cb, LV_EVENT_DELETE, c);
        }
    }
}

/* ===== chrome:// internal pages (Phase G) ===== */
static void render_chrome_page(eb_browser_t *b, const char *url) {
    if (!b || !b->page_area) return;
    lv_obj_clean(b->page_area);

    if (strcmp(url, "chrome://settings") == 0) {
        eb_settings_render(b->page_area);
        return;
    }

    lv_obj_t *cont = lv_obj_create(b->page_area);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, 24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cont, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *h = lv_label_create(cont);
    lv_obj_set_style_text_color(h, EB_COL_BLUE, LV_PART_MAIN);
    lv_obj_set_style_text_font(h, &lv_font_montserrat_14, LV_PART_MAIN);

    if (strcmp(url, "chrome://about") == 0 || strcmp(url, "chrome://version") == 0) {
        lv_label_set_text(h, "About eBrowser");
        lv_obj_t *txt = lv_label_create(cont);
        lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(txt, LV_PCT(100));
        lv_label_set_text_fmt(txt,
            "eBrowser %s\n\n"
            "A lightweight LVGL+SDL2 web browser for embedded and IoT devices.\n"
            "https://github.com/embeddedos-org/eBrowser",
            EB_VERSION);
    } else if (strcmp(url, "chrome://history") == 0) {
        lv_label_set_text(h, "History");
        if (b->history_count == 0) {
            lv_obj_t *e = lv_label_create(cont);
            lv_label_set_text(e, "(no history yet)");
        } else {
            int start = b->history_count - 50;
            if (start < 0) start = 0;
            for (int i = b->history_count - 1; i >= start; i--) {
                lv_obj_t *row = lv_label_create(cont);
                lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
                lv_obj_set_width(row, LV_PCT(100));
                lv_label_set_text(row, b->history[i]);
                lv_obj_set_style_text_color(row, EB_COL_TEXT, LV_PART_MAIN);
            }
        }
    } else if (strcmp(url, "chrome://bookmarks") == 0) {
        lv_label_set_text(h, "Bookmarks");
        for (int i = 0; i < b->bookmarks.count; i++) {
            eb_bookmark_t *bm = &b->bookmarks.items[i];
            if (bm->type != EB_BM_BOOKMARK) continue;
            lv_obj_t *row = lv_label_create(cont);
            lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
            lv_obj_set_width(row, LV_PCT(100));
            lv_label_set_text_fmt(row, "%s — %s",
                bm->title[0] ? bm->title : "(untitled)", bm->url);
            lv_obj_set_style_text_color(row, EB_COL_TEXT, LV_PART_MAIN);
        }
    } else if (strcmp(url, "chrome://downloads") == 0) {
        lv_label_set_text(h, "Downloads");
        if (b->downloads.count == 0) {
            lv_obj_t *e = lv_label_create(cont);
            lv_label_set_text(e, "(no downloads yet)");
        } else {
            for (int i = 0; i < b->downloads.count; i++) {
                eb_download_t *d = &b->downloads.downloads[i];
                lv_obj_t *row = lv_label_create(cont);
                lv_label_set_text_fmt(row, "%s — %d%% (%zu / %zu bytes)",
                    d->filename[0] ? d->filename : d->url,
                    d->progress_percent, d->downloaded_bytes, d->total_bytes);
            }
        }
    } else if (strcmp(url, "chrome://newtab") == 0 ||
               strcmp(url, "about:home") == 0 ||
               strcmp(url, "about:newtab") == 0) {
        /* Inject the home HTML as a quick visual page. */
        lv_obj_clean(b->page_area);
        render_html(b, s_home_html, strlen(s_home_html));
        return;
    } else if (strcmp(url, "chrome://extensions") == 0) {
        lv_label_set_text(h, "Extensions");
        lv_obj_t *e = lv_label_create(cont);
        lv_label_set_text(e, "No extensions installed.");
    } else {
        lv_label_set_text(h, "Unknown chrome:// page");
        lv_obj_t *e = lv_label_create(cont);
        lv_label_set_text_fmt(e, "%s\n\nKnown pages: chrome://newtab, chrome://history, "
            "chrome://bookmarks, chrome://downloads, chrome://settings, "
            "chrome://about, chrome://extensions", url);
    }
}

bool eb_browser_init(eb_browser_t *browser, lv_obj_t *parent) {
    if (!browser || !parent) return false;
    memset(browser, 0, sizeof(eb_browser_t));

    /* Initialize backing managers (Phase B). */
    eb_bm_init(&browser->bookmarks);
    eb_dl_init(&browser->downloads, NULL);
    eb_dt_init(&browser->devtools);
    eb_reader_init(&browser->reader);
    eb_theme_init(&browser->theme);
    eb_priv_init(&browser->privacy);
    eb_settings_init();

    /* Load persisted bookmarks if any. */
    eb_bm_load(&browser->bookmarks, bookmarks_path());
    browser->bookmarks_bar_visible =
        eb_settings_get_bool("appearance.show_bookmarks_bar", true);

    browser->root = lv_obj_create(parent);
    lv_obj_set_size(browser->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(browser->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(browser->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(browser->root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->root, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->root, EB_COL_BG_TOP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->root, LV_OPA_COVER, LV_PART_MAIN);

    /* ----- Tab strip ----- */
    browser->tab_bar = lv_obj_create(browser->root);
    lv_obj_set_size(browser->tab_bar, LV_PCT(100), 38);
    lv_obj_set_flex_flow(browser->tab_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(browser->tab_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_hor(browser->tab_bar, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_top(browser->tab_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(browser->tab_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(browser->tab_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->tab_bar, EB_COL_BG_TOP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->tab_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->tab_bar, 0, LV_PART_MAIN);

    /* ----- Toolbar ----- */
    browser->toolbar = lv_obj_create(browser->root);
    lv_obj_set_size(browser->toolbar, LV_PCT(100), 48);
    lv_obj_set_flex_flow(browser->toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(browser->toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(browser->toolbar, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(browser->toolbar, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(browser->toolbar, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->toolbar, EB_COL_BG_TOOLBAR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->toolbar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->toolbar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(browser->toolbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(browser->toolbar, EB_COL_BORDER_SOFT, LV_PART_MAIN);

    browser->back_btn    = create_nav_button(browser->toolbar, LV_SYMBOL_LEFT,    back_btn_cb,    browser);
    browser->forward_btn = create_nav_button(browser->toolbar, LV_SYMBOL_RIGHT,   forward_btn_cb, browser);
    browser->reload_btn  = create_nav_button(browser->toolbar, LV_SYMBOL_REFRESH, reload_btn_cb,  browser);
    browser->home_btn    = create_nav_button(browser->toolbar, LV_SYMBOL_HOME,    home_btn_cb,    browser);

    browser->url_bar = lv_textarea_create(browser->toolbar);
    lv_textarea_set_one_line(browser->url_bar, true);
    lv_textarea_set_placeholder_text(browser->url_bar, "Search Google or type a URL");
    lv_obj_set_flex_grow(browser->url_bar, 1);
    lv_obj_set_height(browser->url_bar, 36);
    lv_obj_set_style_radius(browser->url_bar, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->url_bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->url_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->url_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(browser->url_bar, EB_COL_BORDER, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(browser->url_bar, 14, LV_PART_MAIN);
    lv_obj_set_style_text_color(browser->url_bar, EB_COL_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(browser->url_bar, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_add_event_cb(browser->url_bar, url_bar_event_cb, LV_EVENT_READY, browser);

    /* Star (bookmark) button — Phase C. */
    browser->star_btn = lv_button_create(browser->toolbar);
    lv_obj_set_size(browser->star_btn, 32, 32);
    lv_obj_set_style_radius(browser->star_btn, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->star_btn, EB_COL_BG_TOOLBAR, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->star_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(browser->star_btn, 0, LV_PART_MAIN);
    browser->star_label = lv_label_create(browser->star_btn);
    lv_label_set_text(browser->star_label, "+");
    lv_obj_set_style_text_color(browser->star_label, EB_COL_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_center(browser->star_label);
    lv_obj_add_event_cb(browser->star_btn, star_btn_cb, LV_EVENT_CLICKED, browser);

    /* Hamburger menu — Phase F (popup panel, never clips). */
    browser->menu_btn = lv_button_create(browser->toolbar);
    browser->menu_dropdown = browser->menu_btn; /* compat */
    lv_obj_set_size(browser->menu_btn, 40, 36);
    lv_obj_set_style_radius(browser->menu_btn, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->menu_btn, EB_COL_BG_TOOLBAR, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->menu_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(browser->menu_btn, 0, LV_PART_MAIN);
    lv_obj_t *mlbl = lv_label_create(browser->menu_btn);
    lv_label_set_text(mlbl, LV_SYMBOL_LIST);
    lv_obj_set_style_text_color(mlbl, EB_COL_TEXT, LV_PART_MAIN);
    lv_obj_center(mlbl);
    lv_obj_add_event_cb(browser->menu_btn, menu_btn_cb, LV_EVENT_CLICKED, browser);

    /* ----- Bookmarks bar (Phase C) ----- */
    browser->bm_bar = lv_obj_create(browser->root);
    lv_obj_set_size(browser->bm_bar, LV_PCT(100), EB_BM_BAR_HEIGHT);
    lv_obj_set_flex_flow(browser->bm_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_hor(browser->bm_bar, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(browser->bm_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(browser->bm_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->bm_bar, EB_COL_STATUS_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->bm_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->bm_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(browser->bm_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(browser->bm_bar, EB_COL_BORDER_SOFT, LV_PART_MAIN);
    if (!browser->bookmarks_bar_visible) lv_obj_add_flag(browser->bm_bar, LV_OBJ_FLAG_HIDDEN);

    /* ----- Page area ----- */
    browser->page_area = lv_obj_create(browser->root);
    lv_obj_set_size(browser->page_area, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(browser->page_area, 1);
    lv_obj_set_style_pad_all(browser->page_area, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->page_area, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->page_area, EB_COL_PAGE_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->page_area, LV_OPA_COVER, LV_PART_MAIN);

    /* ----- Downloads panel (hidden by default) ----- */
    browser->dl_panel = lv_obj_create(browser->root);
    lv_obj_set_size(browser->dl_panel, LV_PCT(100), EB_DL_PANEL_HEIGHT);
    lv_obj_set_style_bg_color(browser->dl_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(browser->dl_panel, EB_COL_BORDER_SOFT, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->dl_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(browser->dl_panel, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_add_flag(browser->dl_panel, LV_OBJ_FLAG_HIDDEN);

    /* ----- DevTools panel (hidden by default) ----- */
    browser->dt_panel = lv_obj_create(browser->root);
    lv_obj_set_size(browser->dt_panel, LV_PCT(100), EB_DT_PANEL_HEIGHT);
    lv_obj_set_style_bg_color(browser->dt_panel, lv_color_make(0x20, 0x21, 0x24), LV_PART_MAIN);
    lv_obj_set_style_border_color(browser->dt_panel, EB_COL_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->dt_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(browser->dt_panel, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_add_flag(browser->dt_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *dthdr = lv_label_create(browser->dt_panel);
    lv_label_set_text(dthdr, "DevTools — Console / Network / Elements (read-only)");
    lv_obj_set_style_text_color(dthdr, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(dthdr, LV_ALIGN_TOP_LEFT, 8, 6);

    /* ----- Status bar ----- */
    browser->status_bar = lv_obj_create(browser->root);
    lv_obj_set_size(browser->status_bar, LV_PCT(100), 22);
    lv_obj_set_style_pad_hor(browser->status_bar, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(browser->status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->status_bar, EB_COL_STATUS_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->status_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->status_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(browser->status_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(browser->status_bar, EB_COL_BORDER_SOFT, LV_PART_MAIN);

    browser->status_label = lv_label_create(browser->status_bar);
    lv_label_set_text(browser->status_label, "Ready");
    lv_obj_set_style_text_font(browser->status_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(browser->status_label, EB_COL_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(browser->status_label, LV_ALIGN_LEFT_MID, 0, 0);

    eb_browser_new_tab(browser);
    rebuild_tab_strip(browser);
    rebuild_bookmarks_bar(browser);

    /* Tick downloads periodically (Phase D). */
    lv_timer_t *dlt = lv_timer_create(dl_tick_cb, 250, browser);
    (void)dlt;

    eb_browser_home(browser);
    return true;
}

void eb_browser_deinit(eb_browser_t *browser) {
    if (!browser) return;
    persist_bookmarks(browser);
    eb_settings_deinit();
    eb_bm_destroy(&browser->bookmarks);
    eb_dl_destroy(&browser->downloads);
    eb_dt_destroy(&browser->devtools);
    eb_reader_destroy(&browser->reader);
    eb_priv_destroy(&browser->privacy);
    if (browser->root) lv_obj_delete(browser->root);
    memset(browser, 0, sizeof(eb_browser_t));
}

static void render_html(eb_browser_t *browser, const char *html, size_t len) {
    if (!browser || !html || len == 0) return;

    lv_obj_clean(browser->page_area);

    eb_dom_node_t *dom = eb_html_parse(html, len);
    if (!dom) {
        eb_browser_set_status(browser, "Error: Failed to parse HTML");
        return;
    }

    /* Reader Mode (Phase J) — render through eb_reader_extract instead of layout. */
    if (browser->reader_mode_active) {
        if (eb_reader_extract(&browser->reader, dom)) {
            lv_obj_t *body = lv_label_create(browser->page_area);
            lv_obj_set_width(body, LV_PCT(80));
            lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 16);
            lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
            if (browser->reader.title[0]) {
                lv_label_set_text_fmt(body, "%s\n\n%s", browser->reader.title,
                                      browser->reader.content ? browser->reader.content : "");
            } else {
                lv_label_set_text(body, browser->reader.content ? browser->reader.content : "");
            }
            eb_dom_destroy(dom);
            eb_browser_set_status(browser, "Reader Mode");
            return;
        }
    }

    eb_stylesheet_t ss;
    eb_css_init_stylesheet(&ss);

    eb_dom_node_t *style_node = eb_dom_find_by_tag(dom, "style");
    if (style_node && style_node->child_count > 0 &&
        style_node->children[0]->type == EB_NODE_TEXT) {
        eb_css_parse(&ss, style_node->children[0]->text, strlen(style_node->children[0]->text));
    }

    eb_layout_context_t layout_ctx;
    int page_w = 1200;
    int page_h = 600;
    eb_layout_init(&layout_ctx, page_w, page_h);

    eb_dom_node_t *body = eb_dom_find_by_tag(dom, "body");
    if (!body) body = dom;

    eb_layout_box_t *root_box = eb_layout_build(&layout_ctx, body, &ss);
    if (root_box) {
        eb_layout_compute(&layout_ctx, root_box);
        eb_render_target_t rt;
        eb_render_init(&rt, browser->page_area, page_w, page_h);
        eb_color_t bg = {255, 255, 255, 255};
        eb_render_clear(&rt, bg);
        eb_render_paint(&rt, root_box);
    }

    eb_dom_node_t *title_node = eb_dom_find_by_tag(dom, "title");
    if (title_node && title_node->child_count > 0 &&
        title_node->children[0]->type == EB_NODE_TEXT) {
        strncpy(browser->tabs[browser->active_tab].title,
                title_node->children[0]->text, 255);
        rebuild_tab_strip(browser);
    }

    eb_dom_destroy(dom);
    eb_browser_set_status(browser, "Done");
}

void eb_browser_navigate(eb_browser_t *browser, const char *url) {
    if (!browser || !url) return;

    /* Apply incognito privacy mode to this navigation. */
    eb_priv_set_mode(&browser->privacy,
        browser->tabs[browser->active_tab].incognito ? EB_PRIV_INCOGNITO : EB_PRIV_NORMAL);

    strncpy(browser->tabs[browser->active_tab].url, url, EB_BROWSER_MAX_URL - 1);
    lv_textarea_set_text(browser->url_bar, url);

    /* Don't store incognito navigations in history. */
    if (!browser->tabs[browser->active_tab].incognito) {
        if (browser->history_pos < browser->history_count - 1) {
            browser->history_count = browser->history_pos + 1;
        }
        if (browser->history_count < EB_BROWSER_MAX_HISTORY) {
            strncpy(browser->history[browser->history_count], url, EB_BROWSER_MAX_URL - 1);
            browser->history_pos = browser->history_count;
            browser->history_count++;
        }
    }

    /* ===== chrome:// internal pages (Phase G) ===== */
    if (strncmp(url, "chrome://", 9) == 0) {
        render_chrome_page(browser, url);
        snprintf(browser->tabs[browser->active_tab].title, 255, "%s", url + 9);
        rebuild_tab_strip(browser);
        update_star_button(browser);
        eb_browser_set_status(browser, url);
        return;
    }

    /* about:home / about:newtab */
    if (strcmp(url, EB_HOME_URL) == 0 || strcmp(url, "about:home") == 0 ||
        strcmp(url, "about:newtab") == 0) {
        eb_browser_set_status(browser, "Loading home page...");
        render_html(browser, s_home_html, strlen(s_home_html));
        strncpy(browser->tabs[browser->active_tab].title, "New Tab", 255);
        rebuild_tab_strip(browser);
        update_star_button(browser);
        return;
    }

    char status[256];
    snprintf(status, sizeof(status), "Loading %s...", url);
    eb_browser_set_status(browser, status);
    browser->tabs[browser->active_tab].loading = true;

    eb_http_response_t resp;
    int rc = eb_http_get(url, &resp);
    if (rc == 0 && resp.body) {
        /* Heuristic: if we got HTML back, try to render it. If render fails or
         * the response doesn't look like something our engine can handle, show
         * the engine-fallback page (Phase I). */
        bool looks_html = false;
        if (resp.content_type && strstr(resp.content_type, "html")) looks_html = true;
        if (!looks_html && resp.body && resp.body_len > 0) {
            if (strstr(resp.body, "<html") || strstr(resp.body, "<HTML") ||
                strstr(resp.body, "<!DOCTYPE")) looks_html = true;
        }

        if (looks_html) {
            render_html(browser, resp.body, resp.body_len);
            snprintf(status, sizeof(status), "HTTP %d — %zu bytes",
                     resp.status_code, resp.body_len);
            eb_browser_set_status(browser, status);
        } else {
            show_engine_fallback_page(browser, url, resp.body, resp.body_len);
            eb_browser_set_status(browser, "Engine fallback (non-HTML response)");
        }

        /* Record in DevTools network panel. */
        if (browser->devtools_open) {
            eb_dt_network_entry_t ne;
            memset(&ne, 0, sizeof(ne));
            strncpy(ne.url, url, sizeof(ne.url) - 1);
            strncpy(ne.method, "GET", sizeof(ne.method) - 1);
            ne.status = resp.status_code;
            ne.res_size = resp.body_len;
            eb_dt_record_request(&browser->devtools, &ne);
        }
        eb_http_response_free(&resp);
    } else {
        /* Phase I: full engine fallback page. */
        show_engine_fallback_page(browser, url, NULL, 0);
        eb_browser_set_status(browser, "Could not connect — showing fallback");
        eb_http_response_free(&resp);
    }

    browser->tabs[browser->active_tab].loading = false;
    update_star_button(browser);
}

void eb_browser_back(eb_browser_t *browser) {
    if (!browser || browser->history_pos <= 0) return;
    browser->history_pos--;
    eb_browser_navigate(browser, browser->history[browser->history_pos]);
}

void eb_browser_forward(eb_browser_t *browser) {
    if (!browser || browser->history_pos >= browser->history_count - 1) return;
    browser->history_pos++;
    eb_browser_navigate(browser, browser->history[browser->history_pos]);
}

void eb_browser_reload(eb_browser_t *browser) {
    if (!browser) return;
    const char *url = browser->tabs[browser->active_tab].url;
    if (url[0]) eb_browser_navigate(browser, url);
}

void eb_browser_home(eb_browser_t *browser) {
    if (!browser) return;
    eb_browser_navigate(browser, EB_HOME_URL);
}

int eb_browser_new_tab(eb_browser_t *browser) {
    if (!browser || browser->tab_count >= EB_BROWSER_MAX_TABS) return -1;
    int idx = browser->tab_count++;
    memset(&browser->tabs[idx], 0, sizeof(eb_tab_t));
    strncpy(browser->tabs[idx].title, "New Tab", 255);
    browser->tabs[idx].zoom_percent = 100;
    browser->active_tab = idx;
    return idx;
}

int eb_browser_new_incognito_tab(eb_browser_t *browser) {
    int idx = eb_browser_new_tab(browser);
    if (idx >= 0) {
        browser->tabs[idx].incognito = true;
        strncpy(browser->tabs[idx].title, "Incognito", 255);
    }
    return idx;
}

void eb_browser_close_tab(eb_browser_t *browser, int index) {
    if (!browser || index < 0 || index >= browser->tab_count) return;
    if (browser->tab_count <= 1) return;
    for (int i = index; i < browser->tab_count - 1; i++) {
        browser->tabs[i] = browser->tabs[i + 1];
    }
    browser->tab_count--;
    if (browser->active_tab >= browser->tab_count) {
        browser->active_tab = browser->tab_count - 1;
    } else if (browser->active_tab > index) {
        browser->active_tab--;
    }
}

void eb_browser_switch_tab(eb_browser_t *browser, int index) {
    if (!browser || index < 0 || index >= browser->tab_count) return;
    browser->active_tab = index;
    lv_textarea_set_text(browser->url_bar, browser->tabs[index].url);
    update_star_button(browser);
}

void eb_browser_set_status(eb_browser_t *browser, const char *text) {
    if (!browser || !browser->status_label) return;
    lv_label_set_text(browser->status_label, text ? text : "");
}

/* ===== Phase F additional helpers ===== */
void eb_browser_focus_omnibox(eb_browser_t *b) {
    if (!b || !b->url_bar) return;
    /* Bring the omnibox into focus and select-all via cursor at end. */
    lv_obj_t *kg = (lv_obj_t *)lv_group_get_focused(lv_group_get_default());
    (void)kg;
    if (lv_group_get_default()) lv_group_focus_obj(b->url_bar);
    lv_textarea_set_cursor_pos(b->url_bar, LV_TEXTAREA_CURSOR_LAST);
}

void eb_browser_zoom_in(eb_browser_t *b) {
    if (!b) return;
    int *z = &b->tabs[b->active_tab].zoom_percent;
    if (*z < 300) *z += 10;
    char st[64]; snprintf(st, sizeof(st), "Zoom: %d%%", *z);
    eb_browser_set_status(b, st);
}
void eb_browser_zoom_out(eb_browser_t *b) {
    if (!b) return;
    int *z = &b->tabs[b->active_tab].zoom_percent;
    if (*z > 30) *z -= 10;
    char st[64]; snprintf(st, sizeof(st), "Zoom: %d%%", *z);
    eb_browser_set_status(b, st);
}
void eb_browser_next_tab(eb_browser_t *b) {
    if (!b || b->tab_count <= 1) return;
    int n = (b->active_tab + 1) % b->tab_count;
    eb_browser_switch_tab(b, n);
    rebuild_tab_strip(b);
    eb_browser_navigate(b, b->tabs[n].url[0] ? b->tabs[n].url : EB_HOME_URL);
}
void eb_browser_prev_tab(eb_browser_t *b) {
    if (!b || b->tab_count <= 1) return;
    int n = (b->active_tab - 1 + b->tab_count) % b->tab_count;
    eb_browser_switch_tab(b, n);
    rebuild_tab_strip(b);
    eb_browser_navigate(b, b->tabs[n].url[0] ? b->tabs[n].url : EB_HOME_URL);
}
