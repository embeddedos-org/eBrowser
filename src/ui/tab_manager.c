// SPDX-License-Identifier: MIT
#include "eBrowser/tab_manager.h"
#include "eBrowser/compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <strings.h>

static uint64_t tab_now(void) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec;
}

void eb_tabmgr_init(eb_tab_manager_t *tm) {
    if (!tm) return; memset(tm, 0, sizeof(*tm));
    tm->active_tab = -1; tm->next_tab_id = 1; tm->next_group_id = 1;
    tm->split_left = tm->split_right = -1;
    tm->memory_limit = 512 * 1024 * 1024; /* 512 MB */
}

void eb_tabmgr_destroy(eb_tab_manager_t *tm) { if (tm) memset(tm, 0, sizeof(*tm)); }

static eb_managed_tab_t *find_tab(eb_tab_manager_t *tm, int id) {
    for (int i = 0; i < tm->tab_count; i++)
        if (tm->tabs[i].id == id) return &tm->tabs[i];
    return NULL;
}

int eb_tabmgr_new(eb_tab_manager_t *tm, const char *url, bool incognito) {
    if (!tm || tm->tab_count >= EB_TAB_MAX) return -1;
    eb_managed_tab_t *t = &tm->tabs[tm->tab_count];
    memset(t, 0, sizeof(*t));
    t->id = tm->next_tab_id++;
    if (url) strncpy(t->url, url, EB_TAB_URL_MAX-1);
    else strcpy(t->url, "about:home");
    strcpy(t->title, "New Tab");
    t->state = EB_TAB_LOADING; t->incognito = incognito;
    t->zoom_percent = 100; t->group_id = -1;
    t->created_at = t->last_accessed = tab_now();
    t->parent_tab_id = tm->active_tab >= 0 ? tm->tabs[tm->active_tab].id : -1;
    tm->tab_count++;
    tm->active_tab = tm->tab_count - 1;
    return t->id;
}

void eb_tabmgr_close(eb_tab_manager_t *tm, int id) {
    if (!tm) return;
    for (int i = 0; i < tm->tab_count; i++) {
        if (tm->tabs[i].id != id) continue;
        /* Save to recently closed */
        if (tm->closed_count < EB_TAB_MAX_CLOSED) {
            eb_closed_tab_t *c = &tm->closed[tm->closed_count++];
            strncpy(c->url, tm->tabs[i].url, EB_TAB_URL_MAX-1);
            strncpy(c->title, tm->tabs[i].title, EB_TAB_TITLE_MAX-1);
            c->closed_at = tab_now(); c->group_id = tm->tabs[i].group_id;
        }
        /* Remove from group */
        if (tm->tabs[i].group_id >= 0) {
            for (int g = 0; g < tm->group_count; g++)
                for (int j = 0; j < tm->groups[g].tab_count; j++)
                    if (tm->groups[g].tab_ids[j] == id) {
                        for (int k = j; k < tm->groups[g].tab_count-1; k++)
                            tm->groups[g].tab_ids[k] = tm->groups[g].tab_ids[k+1];
                        tm->groups[g].tab_count--; break;
                    }
        }
        for (int j = i; j < tm->tab_count-1; j++) tm->tabs[j] = tm->tabs[j+1];
        tm->tab_count--;
        if (tm->active_tab >= tm->tab_count) tm->active_tab = tm->tab_count - 1;
        if (tm->tab_count == 0) eb_tabmgr_new(tm, NULL, false);
        return;
    }
}

void eb_tabmgr_switch(eb_tab_manager_t *tm, int id) {
    if (!tm) return;
    for (int i = 0; i < tm->tab_count; i++)
        if (tm->tabs[i].id == id) {
            tm->active_tab = i;
            tm->tabs[i].last_accessed = tab_now();
            if (tm->tabs[i].hibernated) eb_tabmgr_wake(tm, id);
            return;
        }
}

int eb_tabmgr_duplicate(eb_tab_manager_t *tm, int id) {
    eb_managed_tab_t *src = find_tab(tm, id);
    if (!src) return -1;
    return eb_tabmgr_new(tm, src->url, src->incognito);
}

eb_managed_tab_t *eb_tabmgr_get(eb_tab_manager_t *tm, int id) { return find_tab(tm, id); }

void eb_tabmgr_pin(eb_tab_manager_t *tm, int id, bool pin) {
    eb_managed_tab_t *t = find_tab(tm, id); if (t) t->pinned = pin;
}
void eb_tabmgr_mute(eb_tab_manager_t *tm, int id, bool mute) {
    eb_managed_tab_t *t = find_tab(tm, id); if (t) t->muted = mute;
}
void eb_tabmgr_set_zoom(eb_tab_manager_t *tm, int id, int pct) {
    eb_managed_tab_t *t = find_tab(tm, id);
    if (t && pct >= 25 && pct <= 500) t->zoom_percent = pct;
}

int eb_tabmgr_create_group(eb_tab_manager_t *tm, const char *name, int color) {
    if (!tm || tm->group_count >= EB_TAB_MAX_GROUPS) return -1;
    eb_tab_group_t *g = &tm->groups[tm->group_count];
    memset(g, 0, sizeof(*g));
    g->id = tm->next_group_id++;
    if (name) strncpy(g->name, name, 63);
    g->color = color;
    tm->group_count++;
    return g->id;
}

bool eb_tabmgr_add_to_group(eb_tab_manager_t *tm, int tid, int gid) {
    if (!tm) return false;
    eb_managed_tab_t *t = find_tab(tm, tid); if (!t) return false;
    for (int i = 0; i < tm->group_count; i++) {
        if (tm->groups[i].id != gid) continue;
        if (tm->groups[i].tab_count >= EB_TAB_MAX) return false;
        t->group_id = gid;
        tm->groups[i].tab_ids[tm->groups[i].tab_count++] = tid;
        return true;
    }
    return false;
}

void eb_tabmgr_collapse_group(eb_tab_manager_t *tm, int gid, bool c) {
    if (!tm) return;
    for (int i = 0; i < tm->group_count; i++)
        if (tm->groups[i].id == gid) { tm->groups[i].collapsed = c; return; }
}

void eb_tabmgr_hibernate(eb_tab_manager_t *tm, int id) {
    eb_managed_tab_t *t = find_tab(tm, id);
    if (t && !t->pinned) { t->hibernated = true; t->state = EB_TAB_HIBERNATED; t->memory_usage = 0; }
}

void eb_tabmgr_wake(eb_tab_manager_t *tm, int id) {
    eb_managed_tab_t *t = find_tab(tm, id);
    if (t && t->hibernated) { t->hibernated = false; t->state = EB_TAB_LOADING; }
}

void eb_tabmgr_auto_hibernate(eb_tab_manager_t *tm, int sec) {
    if (!tm) return;
    uint64_t now = tab_now();
    for (int i = 0; i < tm->tab_count; i++) {
        if (i == tm->active_tab) continue;
        if (tm->tabs[i].pinned || tm->tabs[i].hibernated) continue;
        if (now - tm->tabs[i].last_accessed > (uint64_t)sec)
            eb_tabmgr_hibernate(tm, tm->tabs[i].id);
    }
}

void eb_tabmgr_split(eb_tab_manager_t *tm, int l, int r) {
    if (!tm) return; tm->split_left = l; tm->split_right = r; tm->split_active = true;
}
void eb_tabmgr_unsplit(eb_tab_manager_t *tm) {
    if (!tm) return; tm->split_active = false; tm->split_left = tm->split_right = -1;
}

int eb_tabmgr_reopen_closed(eb_tab_manager_t *tm) {
    if (!tm || tm->closed_count == 0) return -1;
    eb_closed_tab_t *c = &tm->closed[--tm->closed_count];
    return eb_tabmgr_new(tm, c->url, false);
}

int eb_tabmgr_search(const eb_tab_manager_t *tm, const char *q, int *res, int max) {
    if (!tm || !q || !res) return 0;
    int n = 0;
    for (int i = 0; i < tm->tab_count && n < max; i++)
        if (strcasestr(tm->tabs[i].title, q) || strcasestr(tm->tabs[i].url, q))
            res[n++] = tm->tabs[i].id;
    return n;
}
