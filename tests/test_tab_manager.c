// SPDX-License-Identifier: MIT
#include "eBrowser/tab_manager.h"
#include <stdio.h>
#include <string.h>

static int s_pass = 0, s_fail = 0;
#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("PASS\n"); s_pass++; } while(0)
#define ASSERT(cond) do { if(!(cond)) { printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); s_fail++; return; } } while(0)

TEST(test_init) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    ASSERT(tm.tab_count == 0);
    ASSERT(tm.active_tab == -1);
    ASSERT(tm.next_tab_id == 1);
    ASSERT(tm.group_count == 0);
    ASSERT(tm.split_active == false);
    eb_tabmgr_destroy(&tm);
}

TEST(test_new_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://example.com", false);
    ASSERT(id > 0);
    ASSERT(tm.tab_count == 1);
    ASSERT(tm.active_tab == 0);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, id);
    ASSERT(t != NULL);
    ASSERT(strcmp(t->url, "https://example.com") == 0);
    ASSERT(t->incognito == false);
    ASSERT(t->zoom_percent == 100);
    ASSERT(t->state == EB_TAB_LOADING);
    eb_tabmgr_destroy(&tm);
}

TEST(test_new_tab_default_url) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, NULL, false);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, id);
    ASSERT(t != NULL);
    ASSERT(strcmp(t->url, "about:home") == 0);
    eb_tabmgr_destroy(&tm);
}

TEST(test_incognito_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://private.com", true);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, id);
    ASSERT(t != NULL);
    ASSERT(t->incognito == true);
    eb_tabmgr_destroy(&tm);
}

TEST(test_multiple_tabs) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int a = eb_tabmgr_new(&tm, "https://a.com", false);
    int b = eb_tabmgr_new(&tm, "https://b.com", false);
    int c = eb_tabmgr_new(&tm, "https://c.com", false);
    ASSERT(tm.tab_count == 3);
    ASSERT(a != b && b != c);
    /* Last tab is active */
    ASSERT(tm.active_tab == 2);
    eb_tabmgr_destroy(&tm);
}

TEST(test_switch_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int a = eb_tabmgr_new(&tm, "https://a.com", false);
    int b = eb_tabmgr_new(&tm, "https://b.com", false);
    eb_tabmgr_switch(&tm, a);
    ASSERT(tm.active_tab == 0);
    eb_tabmgr_switch(&tm, b);
    ASSERT(tm.active_tab == 1);
    eb_tabmgr_destroy(&tm);
}

TEST(test_close_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int a = eb_tabmgr_new(&tm, "https://a.com", false);
    int b = eb_tabmgr_new(&tm, "https://b.com", false);
    ASSERT(tm.tab_count == 2);
    eb_tabmgr_close(&tm, a);
    ASSERT(tm.tab_count == 1);
    ASSERT(tm.tabs[0].id == b);
    eb_tabmgr_destroy(&tm);
}

TEST(test_close_last_tab_creates_new) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://x.com", false);
    eb_tabmgr_close(&tm, id);
    ASSERT(tm.tab_count == 1); /* New tab auto-created */
    ASSERT(strcmp(tm.tabs[0].url, "about:home") == 0);
    eb_tabmgr_destroy(&tm);
}

TEST(test_close_saves_to_recently_closed) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    eb_tabmgr_new(&tm, "https://keep.com", false);
    int id = eb_tabmgr_new(&tm, "https://closed.com", false);
    eb_tabmgr_close(&tm, id);
    ASSERT(tm.closed_count == 1);
    ASSERT(strcmp(tm.closed[0].url, "https://closed.com") == 0);
    eb_tabmgr_destroy(&tm);
}

TEST(test_reopen_closed) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    eb_tabmgr_new(&tm, "https://keep.com", false);
    int id = eb_tabmgr_new(&tm, "https://reopen.com", false);
    eb_tabmgr_close(&tm, id);
    ASSERT(tm.closed_count == 1);
    int reopened = eb_tabmgr_reopen_closed(&tm);
    ASSERT(reopened > 0);
    ASSERT(tm.closed_count == 0);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, reopened);
    ASSERT(t != NULL);
    ASSERT(strcmp(t->url, "https://reopen.com") == 0);
    eb_tabmgr_destroy(&tm);
}

TEST(test_duplicate_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int a = eb_tabmgr_new(&tm, "https://dup.com", false);
    int b = eb_tabmgr_duplicate(&tm, a);
    ASSERT(b > 0 && b != a);
    ASSERT(tm.tab_count == 2);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, b);
    ASSERT(strcmp(t->url, "https://dup.com") == 0);
    eb_tabmgr_destroy(&tm);
}

TEST(test_pin_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://pin.com", false);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, id);
    ASSERT(t->pinned == false);
    eb_tabmgr_pin(&tm, id, true);
    ASSERT(t->pinned == true);
    eb_tabmgr_pin(&tm, id, false);
    ASSERT(t->pinned == false);
    eb_tabmgr_destroy(&tm);
}

TEST(test_mute_tab) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://video.com", false);
    eb_tabmgr_mute(&tm, id, true);
    ASSERT(eb_tabmgr_get(&tm, id)->muted == true);
    eb_tabmgr_mute(&tm, id, false);
    ASSERT(eb_tabmgr_get(&tm, id)->muted == false);
    eb_tabmgr_destroy(&tm);
}

TEST(test_zoom) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://x.com", false);
    eb_tabmgr_set_zoom(&tm, id, 150);
    ASSERT(eb_tabmgr_get(&tm, id)->zoom_percent == 150);
    eb_tabmgr_set_zoom(&tm, id, 10); /* Too small, should not change */
    ASSERT(eb_tabmgr_get(&tm, id)->zoom_percent == 150);
    eb_tabmgr_set_zoom(&tm, id, 600); /* Too large */
    ASSERT(eb_tabmgr_get(&tm, id)->zoom_percent == 150);
    eb_tabmgr_destroy(&tm);
}

TEST(test_tab_groups) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int t1 = eb_tabmgr_new(&tm, "https://a.com", false);
    int t2 = eb_tabmgr_new(&tm, "https://b.com", false);
    int gid = eb_tabmgr_create_group(&tm, "Work", 0x4285F4);
    ASSERT(gid > 0);
    ASSERT(tm.group_count == 1);
    ASSERT(strcmp(tm.groups[0].name, "Work") == 0);
    ASSERT(tm.groups[0].color == 0x4285F4);
    ASSERT(eb_tabmgr_add_to_group(&tm, t1, gid) == true);
    ASSERT(eb_tabmgr_add_to_group(&tm, t2, gid) == true);
    ASSERT(tm.groups[0].tab_count == 2);
    ASSERT(eb_tabmgr_get(&tm, t1)->group_id == gid);
    eb_tabmgr_destroy(&tm);
}

TEST(test_group_collapse) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int gid = eb_tabmgr_create_group(&tm, "Social", 0xFF0000);
    ASSERT(tm.groups[0].collapsed == false);
    eb_tabmgr_collapse_group(&tm, gid, true);
    ASSERT(tm.groups[0].collapsed == true);
    eb_tabmgr_collapse_group(&tm, gid, false);
    ASSERT(tm.groups[0].collapsed == false);
    eb_tabmgr_destroy(&tm);
}

TEST(test_hibernate) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://idle.com", false);
    eb_managed_tab_t *t = eb_tabmgr_get(&tm, id);
    ASSERT(t->hibernated == false);
    eb_tabmgr_hibernate(&tm, id);
    ASSERT(t->hibernated == true);
    ASSERT(t->state == EB_TAB_HIBERNATED);
    ASSERT(t->memory_usage == 0);
    eb_tabmgr_wake(&tm, id);
    ASSERT(t->hibernated == false);
    ASSERT(t->state == EB_TAB_LOADING);
    eb_tabmgr_destroy(&tm);
}

TEST(test_pinned_tab_no_hibernate) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int id = eb_tabmgr_new(&tm, "https://pinned.com", false);
    eb_tabmgr_pin(&tm, id, true);
    eb_tabmgr_hibernate(&tm, id);
    ASSERT(eb_tabmgr_get(&tm, id)->hibernated == false); /* Pinned tabs don't hibernate */
    eb_tabmgr_destroy(&tm);
}

TEST(test_split_view) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    int a = eb_tabmgr_new(&tm, "https://left.com", false);
    int b = eb_tabmgr_new(&tm, "https://right.com", false);
    eb_tabmgr_split(&tm, a, b);
    ASSERT(tm.split_active == true);
    ASSERT(tm.split_left == a);
    ASSERT(tm.split_right == b);
    eb_tabmgr_unsplit(&tm);
    ASSERT(tm.split_active == false);
    ASSERT(tm.split_left == -1);
    eb_tabmgr_destroy(&tm);
}

TEST(test_search_tabs) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    eb_tabmgr_new(&tm, "https://github.com", false);
    eb_tabmgr_new(&tm, "https://google.com", false);
    eb_tabmgr_new(&tm, "https://example.com", false);
    /* Update titles */
    strcpy(tm.tabs[0].title, "GitHub");
    strcpy(tm.tabs[1].title, "Google Search");
    strcpy(tm.tabs[2].title, "Example");
    int results[10]; int n;
    n = eb_tabmgr_search(&tm, "goo", results, 10);
    ASSERT(n == 1); /* only google matches "goo" */
    n = eb_tabmgr_search(&tm, "example", results, 10);
    ASSERT(n == 1);
    n = eb_tabmgr_search(&tm, "nonexistent", results, 10);
    ASSERT(n == 0);
    eb_tabmgr_destroy(&tm);
}

TEST(test_get_nonexistent) {
    static eb_tab_manager_t tm;
    eb_tabmgr_init(&tm);
    ASSERT(eb_tabmgr_get(&tm, 999) == NULL);
    eb_tabmgr_destroy(&tm);
}

int main(void) {
    printf("=== Tab Manager Tests ===\n");
    RUN(test_init); RUN(test_new_tab); RUN(test_new_tab_default_url);
    RUN(test_incognito_tab); RUN(test_multiple_tabs); RUN(test_switch_tab);
    RUN(test_close_tab); RUN(test_close_last_tab_creates_new);
    RUN(test_close_saves_to_recently_closed); RUN(test_reopen_closed);
    RUN(test_duplicate_tab); RUN(test_pin_tab); RUN(test_mute_tab);
    RUN(test_zoom); RUN(test_tab_groups); RUN(test_group_collapse);
    RUN(test_hibernate); RUN(test_pinned_tab_no_hibernate);
    RUN(test_split_view); RUN(test_search_tabs); RUN(test_get_nonexistent);
    printf("\nResults: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
