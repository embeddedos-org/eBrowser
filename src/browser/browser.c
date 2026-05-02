// SPDX-License-Identifier: MIT
#include "eBrowser/browser.h"
#include "eBrowser/html_parser.h"
#include "eBrowser/css_parser.h"
#include "eBrowser/layout.h"
#include "eBrowser/render.h"
#include "eBrowser/http.h"
#include "eBrowser/url.h"
#include "eb_version.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define EB_HOME_URL    "about:home"
#define EB_DEFAULT_URL "https://example.com"

/* ===== Chrome-inspired palette ===== */
/* Light theme — matches modern Chrome's default look. */
#define EB_COL_BG_TOP        lv_color_make(0xDE, 0xE1, 0xE6) /* tab strip background     */
#define EB_COL_BG_TOOLBAR    lv_color_make(0xF1, 0xF3, 0xF4) /* omnibox / toolbar row    */
#define EB_COL_TAB_INACTIVE  lv_color_make(0xCD, 0xD1, 0xD6) /* inactive tabs            */
#define EB_COL_TAB_HOVER     lv_color_make(0xE2, 0xE5, 0xE9)
#define EB_COL_TAB_ACTIVE    lv_color_make(0xFF, 0xFF, 0xFF) /* active tab merges w/page */
#define EB_COL_PAGE_BG       lv_color_make(0xFF, 0xFF, 0xFF)
#define EB_COL_BORDER        lv_color_make(0xC0, 0xC4, 0xC8)
#define EB_COL_BORDER_SOFT   lv_color_make(0xE0, 0xE2, 0xE5)
#define EB_COL_TEXT          lv_color_make(0x20, 0x21, 0x24)
#define EB_COL_TEXT_MUTED    lv_color_make(0x5F, 0x63, 0x68)
#define EB_COL_BLUE          lv_color_make(0x1A, 0x73, 0xE8) /* Chrome blue */
#define EB_COL_RED           lv_color_make(0xD9, 0x3A, 0x26)
#define EB_COL_STATUS_BG     lv_color_make(0xF8, 0xF9, 0xFA)

#define EB_TAB_WIDTH       200
#define EB_TAB_HEIGHT      32
#define EB_NAV_BTN_SIZE    32

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

/* ===== Forward declarations of static helpers ===== */
static void rebuild_tab_strip(eb_browser_t *b);
static void show_about_dialog(eb_browser_t *b);
static void show_history_dialog(eb_browser_t *b);
static void show_bookmarks_dialog(eb_browser_t *b);

/* ===== Generic event callbacks ===== */
static void url_bar_event_cb(lv_event_t *e) {
    eb_browser_t *browser = (eb_browser_t *)lv_event_get_user_data(e);
    if (!browser) return;
    const char *url = lv_textarea_get_text(browser->url_bar);
    if (url && url[0]) eb_browser_navigate(browser, url);
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

/* ===== Tab strip events ===== */
typedef struct {
    eb_browser_t *browser;
    int           index;
} eb_tab_ctx_t;

/* Tab contexts are owned by the parent; LVGL frees them when the obj is deleted. */
static void tab_ctx_free_cb(lv_event_t *e) {
    eb_tab_ctx_t *ctx = (eb_tab_ctx_t *)lv_event_get_user_data(e);
    if (ctx) free(ctx);
}

static void tab_clicked_cb(lv_event_t *e) {
    eb_tab_ctx_t *ctx = (eb_tab_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->browser) return;
    eb_browser_switch_tab(ctx->browser, ctx->index);
    rebuild_tab_strip(ctx->browser);
    /* Re-render the page for the newly active tab. */
    const char *url = ctx->browser->tabs[ctx->browser->active_tab].url;
    if (url && url[0]) eb_browser_navigate(ctx->browser, url);
    else eb_browser_home(ctx->browser);
}

static void tab_close_cb(lv_event_t *e) {
    eb_tab_ctx_t *ctx = (eb_tab_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->browser) return;
    eb_browser_t *b = ctx->browser;
    int idx = ctx->index;
    /* Only allow closing if more than one tab exists (matches Chrome behavior of
     * keeping at least one tab; closing the last tab in real Chrome quits). */
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

/* ===== Hamburger menu ===== */
enum {
    EB_MENU_NEW_TAB = 0,
    EB_MENU_CLOSE_TAB,
    EB_MENU_BOOKMARKS,
    EB_MENU_HISTORY,
    EB_MENU_ABOUT,
    EB_MENU_QUIT,
};

static void menu_event_cb(lv_event_t *e) {
    eb_browser_t *b = (eb_browser_t *)lv_event_get_user_data(e);
    lv_obj_t *dd = lv_event_get_target(e);
    if (!b || !dd) return;
    uint16_t sel = lv_dropdown_get_selected(dd);
    /* Reset visible label back to ☰ so the dropdown always reads as a menu button. */
    lv_dropdown_set_text(dd, LV_SYMBOL_LIST);

    switch (sel) {
        case EB_MENU_NEW_TAB:
            if (eb_browser_new_tab(b) >= 0) {
                rebuild_tab_strip(b);
                eb_browser_home(b);
            }
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
        case EB_MENU_BOOKMARKS:
            show_bookmarks_dialog(b);
            break;
        case EB_MENU_HISTORY:
            show_history_dialog(b);
            break;
        case EB_MENU_ABOUT:
            show_about_dialog(b);
            break;
        case EB_MENU_QUIT:
            b->quit_requested = true;
            break;
        default:
            break;
    }
}

/* ===== Dialogs ===== */
static void show_about_dialog(eb_browser_t *b) {
    (void)b;
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox, "About eBrowser");
    char body[256];
    snprintf(body, sizeof(body),
             "eBrowser %s\n\n"
             "A lightweight web browser for embedded and IoT devices.\n\n"
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
        /* Show the most recent 10 entries. */
        char buf[1024];
        size_t pos = 0;
        int start = b->history_count - 10;
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

static void show_bookmarks_dialog(eb_browser_t *b) {
    (void)b;
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox, "Bookmarks");
    lv_msgbox_add_text(mbox,
        "Bookmarks UI coming soon.\n\n"
        "The backend (eb_bookmark_mgr_t) is built and tested in src/ui/, "
        "but is not yet wired into the desktop UI.");
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
    lv_obj_t *tab = lv_button_create(parent);
    lv_obj_set_size(tab, EB_TAB_WIDTH, EB_TAB_HEIGHT);
    lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(tab, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(tab, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(tab, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab, active ? EB_COL_TAB_ACTIVE : EB_COL_TAB_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Title — truncate with ellipsis like Chrome. */
    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, EB_TAB_WIDTH - 48);
    lv_label_set_text(title, b->tabs[idx].title[0] ? b->tabs[idx].title : "New Tab");
    lv_obj_set_style_text_color(title, active ? EB_COL_TEXT : EB_COL_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, LV_PART_MAIN);

    /* × close button — child of the tab so layout flows. */
    lv_obj_t *close = lv_button_create(tab);
    lv_obj_set_size(close, 18, 18);
    lv_obj_set_style_radius(close, 9, LV_PART_MAIN);
    lv_obj_set_style_pad_all(close, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(close, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(close, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close, 0, LV_PART_MAIN);

    lv_obj_t *xlbl = lv_label_create(close);
    lv_label_set_text(xlbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(xlbl, EB_COL_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(xlbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(xlbl);

    /* Wire body click + close click. Allocate a context with the tab index;
     * free it via LV_EVENT_DELETE so it lives for the tab obj's lifetime. */
    eb_tab_ctx_t *body_ctx = (eb_tab_ctx_t *)calloc(1, sizeof(eb_tab_ctx_t));
    body_ctx->browser = b;
    body_ctx->index   = idx;
    lv_obj_add_event_cb(tab, tab_clicked_cb, LV_EVENT_CLICKED, body_ctx);
    lv_obj_add_event_cb(tab, tab_ctx_free_cb, LV_EVENT_DELETE, body_ctx);

    eb_tab_ctx_t *close_ctx = (eb_tab_ctx_t *)calloc(1, sizeof(eb_tab_ctx_t));
    close_ctx->browser = b;
    close_ctx->index   = idx;
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

    /* + button at the right end (Chrome's "new tab" affordance). */
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

void eb_browser_refresh_tabs(eb_browser_t *browser) {
    rebuild_tab_strip(browser);
}

bool eb_browser_init(eb_browser_t *browser, lv_obj_t *parent) {
    if (!browser || !parent) return false;
    memset(browser, 0, sizeof(eb_browser_t));

    /* Whole-screen vertical layout: [tab strip][toolbar][page area][status bar]. */
    browser->root = lv_obj_create(parent);
    lv_obj_set_size(browser->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(browser->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(browser->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(browser->root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->root, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->root, EB_COL_BG_TOP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->root, LV_OPA_COVER, LV_PART_MAIN);

    /* ----- Tab strip (top, like Chrome) ----- */
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

    /* ----- Toolbar (nav buttons + url bar + menu) ----- */
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

    /* Omnibox (URL bar) — Chrome-style rounded pill, grows to fill. */
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

    /* Hamburger menu — implemented as an lv_dropdown with a fixed ☰ label. */
    browser->menu_btn = lv_dropdown_create(browser->toolbar);
    browser->menu_dropdown = browser->menu_btn;
    lv_dropdown_set_options_static(browser->menu_btn,
        "New Tab\n"
        "Close Tab\n"
        "Bookmarks\n"
        "History\n"
        "About\n"
        "Quit");
    lv_dropdown_set_text(browser->menu_btn, LV_SYMBOL_LIST);
    lv_obj_set_size(browser->menu_btn, 40, 36);
    lv_obj_set_style_radius(browser->menu_btn, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->menu_btn, EB_COL_BG_TOOLBAR, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->menu_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(browser->menu_btn, EB_COL_TEXT, LV_PART_MAIN);
    lv_obj_add_event_cb(browser->menu_btn, menu_event_cb, LV_EVENT_VALUE_CHANGED, browser);

    /* ----- Page area ----- */
    browser->page_area = lv_obj_create(browser->root);
    lv_obj_set_size(browser->page_area, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(browser->page_area, 1);
    lv_obj_set_style_pad_all(browser->page_area, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(browser->page_area, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(browser->page_area, EB_COL_PAGE_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(browser->page_area, LV_OPA_COVER, LV_PART_MAIN);

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
    eb_browser_home(browser);

    return true;
}

void eb_browser_deinit(eb_browser_t *browser) {
    if (!browser) return;
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

    strncpy(browser->tabs[browser->active_tab].url, url, EB_BROWSER_MAX_URL - 1);
    lv_textarea_set_text(browser->url_bar, url);

    if (browser->history_pos < browser->history_count - 1) {
        browser->history_count = browser->history_pos + 1;
    }
    if (browser->history_count < EB_BROWSER_MAX_HISTORY) {
        strncpy(browser->history[browser->history_count], url, EB_BROWSER_MAX_URL - 1);
        browser->history_pos = browser->history_count;
        browser->history_count++;
    }

    if (strcmp(url, EB_HOME_URL) == 0 || strcmp(url, "about:home") == 0) {
        eb_browser_set_status(browser, "Loading home page...");
        render_html(browser, s_home_html, strlen(s_home_html));
        strncpy(browser->tabs[browser->active_tab].title, "New Tab", 255);
        rebuild_tab_strip(browser);
        return;
    }

    char status[256];
    snprintf(status, sizeof(status), "Loading %s...", url);
    eb_browser_set_status(browser, status);

    browser->tabs[browser->active_tab].loading = true;

    eb_http_response_t resp;
    if (eb_http_get(url, &resp) == 0 && resp.body) {
        render_html(browser, resp.body, resp.body_len);
        snprintf(status, sizeof(status), "HTTP %d — %zu bytes", resp.status_code, resp.body_len);
        eb_browser_set_status(browser, status);
        eb_http_response_free(&resp);
    } else {
        lv_obj_clean(browser->page_area);
        lv_obj_t *err_label = lv_label_create(browser->page_area);
        lv_label_set_text(err_label, "Failed to load page.\n\nCheck the URL and try again.");
        lv_obj_set_style_text_color(err_label, EB_COL_RED, LV_PART_MAIN);
        lv_obj_center(err_label);
        eb_browser_set_status(browser, "Error: Could not connect");
        eb_http_response_free(&resp);
    }

    browser->tabs[browser->active_tab].loading = false;
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
    browser->active_tab = idx;
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
}

void eb_browser_set_status(eb_browser_t *browser, const char *text) {
    if (!browser || !browser->status_label) return;
    lv_label_set_text(browser->status_label, text ? text : "");
}
