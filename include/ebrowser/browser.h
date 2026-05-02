// SPDX-License-Identifier: MIT
#ifndef eBrowser_BROWSER_H
#define eBrowser_BROWSER_H

#include "lvgl.h"
#include "eBrowser/bookmark.h"
#include "eBrowser/download.h"
#include "eBrowser/devtools.h"
#include "eBrowser/reader_mode.h"
#include "eBrowser/theme.h"
#include "eBrowser/privacy.h"
#include <stdbool.h>

#define EB_BROWSER_MAX_TABS    16
#define EB_BROWSER_MAX_HISTORY 256
#define EB_BROWSER_MAX_URL     2048

typedef struct {
    char    url[EB_BROWSER_MAX_URL];
    char    title[256];
    bool    loading;
    bool    incognito;
    int     scroll_y;
    int     zoom_percent;
} eb_tab_t;

typedef struct {
    /* ----- Core layout ----- */
    lv_obj_t   *root;
    lv_obj_t   *tab_bar;       /* Top tab strip */
    lv_obj_t   *toolbar;       /* Nav buttons + omnibox + menu */
    lv_obj_t   *bm_bar;        /* Bookmarks bar (Phase C) */
    lv_obj_t   *page_area;     /* Web content area */
    lv_obj_t   *dl_panel;      /* Downloads slide-up panel (Phase D) */
    lv_obj_t   *dt_panel;      /* DevTools panel (Phase J) */
    lv_obj_t   *status_bar;
    lv_obj_t   *status_label;
    lv_obj_t   *menu_panel;    /* Floating popup menu (Phase F) — NULL when hidden */

    /* ----- Toolbar widgets ----- */
    lv_obj_t   *back_btn;
    lv_obj_t   *forward_btn;
    lv_obj_t   *reload_btn;
    lv_obj_t   *home_btn;
    lv_obj_t   *url_bar;
    lv_obj_t   *star_btn;      /* Bookmark this page (Phase C) */
    lv_obj_t   *star_label;    /* ☆ / ★ icon child */
    lv_obj_t   *menu_btn;      /* Hamburger menu */
    lv_obj_t   *menu_dropdown; /* Compatibility alias for menu_btn (kept for tests). */
    lv_obj_t   *incognito_pill; /* Top-right pill shown in incognito tabs (Phase J) */

    /* ----- State ----- */
    eb_tab_t    tabs[EB_BROWSER_MAX_TABS];
    int         tab_count;
    int         active_tab;
    char        history[EB_BROWSER_MAX_HISTORY][EB_BROWSER_MAX_URL];
    int         history_count;
    int         history_pos;
    bool        bookmarks_bar_visible;
    bool        downloads_panel_open;
    bool        devtools_open;
    bool        reader_mode_active;
    bool        quit_requested;

    /* ----- Backing managers (eb_ui modules — Phase B wire-up) ----- */
    eb_bookmark_mgr_t  bookmarks;
    eb_download_mgr_t  downloads;
    eb_devtools_t      devtools;
    eb_reader_mode_t   reader;
    eb_theme_t         theme;
    eb_privacy_t       privacy;
} eb_browser_t;

bool eb_browser_init(eb_browser_t *browser, lv_obj_t *parent);
void eb_browser_deinit(eb_browser_t *browser);
void eb_browser_navigate(eb_browser_t *browser, const char *url);
void eb_browser_back(eb_browser_t *browser);
void eb_browser_forward(eb_browser_t *browser);
void eb_browser_reload(eb_browser_t *browser);
void eb_browser_home(eb_browser_t *browser);
int  eb_browser_new_tab(eb_browser_t *browser);
int  eb_browser_new_incognito_tab(eb_browser_t *browser);
void eb_browser_close_tab(eb_browser_t *browser, int index);
void eb_browser_switch_tab(eb_browser_t *browser, int index);
void eb_browser_set_status(eb_browser_t *browser, const char *text);

/* ----- Helpers added by phases C–J ----- */
void eb_browser_refresh_tabs(eb_browser_t *browser);
void eb_browser_refresh_bookmarks_bar(eb_browser_t *browser);
void eb_browser_toggle_bookmarks_bar(eb_browser_t *browser);
void eb_browser_toggle_bookmark_current(eb_browser_t *browser);
void eb_browser_toggle_downloads_panel(eb_browser_t *browser);
void eb_browser_toggle_devtools(eb_browser_t *browser);
void eb_browser_toggle_reader_mode(eb_browser_t *browser);
void eb_browser_focus_omnibox(eb_browser_t *browser);
void eb_browser_zoom_in(eb_browser_t *browser);
void eb_browser_zoom_out(eb_browser_t *browser);
void eb_browser_next_tab(eb_browser_t *browser);
void eb_browser_prev_tab(eb_browser_t *browser);

#endif /* eBrowser_BROWSER_H */
